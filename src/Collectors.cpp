#include "Collectors.h"
#include "CollectorFactory.h"
#include "Logger.h"
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>

// ========================================
// 工厂模式：自动注册所有采集器
// 添加新采集器时，只需在这里添加一行即可
// ========================================
REGISTER_COLLECTOR(SystemCollector);
REGISTER_COLLECTOR(CPUCollector);
REGISTER_COLLECTOR(MemoryCollector);
REGISTER_COLLECTOR(DiskCollector);
REGISTER_COLLECTOR(NetworkCollector);
REGISTER_COLLECTOR(ProcessCollector);

// ==================== 辅助函数 ====================
namespace {
std::string format_bytes(uint64_t bytes) {
  std::ostringstream oss;
  if (bytes >= 1024ULL * 1024 * 1024) {
    oss << std::fixed << std::setprecision(2)
        << (bytes / 1024.0 / 1024.0 / 1024.0) << " GB";
  } else if (bytes >= 1024 * 1024) {
    oss << std::fixed << std::setprecision(2) << (bytes / 1024.0 / 1024.0)
        << " MB";
  } else if (bytes >= 1024) {
    oss << std::fixed << std::setprecision(2) << (bytes / 1024.0) << " KB";
  } else {
    oss << bytes << " B";
  }
  return oss.str();
}

std::string format_kb(uint64_t kb) {
  std::ostringstream oss;
  if (kb >= 1024 * 1024) {
    oss << std::fixed << std::setprecision(2) << (kb / 1024.0 / 1024.0)
        << " GB";
  } else if (kb >= 1024) {
    oss << std::fixed << std::setprecision(2) << (kb / 1024.0) << " MB";
  } else {
    oss << kb << " KB";
  }
  return oss.str();
}
} // namespace

// ==================== CPUCollector ====================
void CPUCollector::do_collect() {
  std::ifstream file(CPU_PATH);
  if (!file.is_open()) {
    LOG_ERROR("无法打开 " CPU_PATH);
    return;
  }
  std::getline(file, raw_data_);
  file.close();
}

void CPUCollector::do_parse() {
  if (raw_data_.empty())
    return;

  // 保存之前的值
  prev_idle_ = curr_idle_;
  prev_total_ = curr_total_;

  std::stringstream ss(raw_data_);
  std::string cpu_label;
  unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;

  ss >> cpu_label >> user >> nice >> system >> idle >> iowait >> irq >>
      softirq >> steal;

  curr_idle_ = idle + iowait;
  curr_total_ = user + nice + system + idle + iowait + irq + softirq + steal;
}

void CPUCollector::do_calculate() {
  unsigned long long total_diff = curr_total_ - prev_total_;
  unsigned long long idle_diff = curr_idle_ - prev_idle_;

  if (total_diff > 0) {
    usage_percent_ = 100.0 * (total_diff - idle_diff) / total_diff;
  }
}

void CPUCollector::print_result() const {
  std::cout << "CPU 使用率: " << std::fixed << std::setprecision(1)
            << usage_percent_ << "%" << std::endl;
}

// ==================== MemoryCollector ====================
void MemoryCollector::do_collect() {
  std::ifstream file(MEMORY_PATH);
  if (!file.is_open()) {
    LOG_ERROR("无法打开 " MEMORY_PATH);
    return;
  }

  std::ostringstream oss;
  oss << file.rdbuf();
  raw_data_ = oss.str();
  file.close();
}

void MemoryCollector::do_parse() {
  std::istringstream iss(raw_data_);
  std::string line;

  while (std::getline(iss, line)) {
    std::stringstream ss(line);
    std::string key;
    uint64_t value;
    std::string unit;

    ss >> key >> value >> unit;

    if (key == "MemTotal:")
      total_kb_ = value;
    else if (key == "MemFree:")
      free_kb_ = value;
    else if (key == "MemAvailable:")
      available_kb_ = value;
    else if (key == "Buffers:")
      buffers_kb_ = value;
    else if (key == "Cached:")
      cached_kb_ = value;
  }
}

void MemoryCollector::do_calculate() {
  used_kb_ = total_kb_ - available_kb_;
  if (total_kb_ > 0) {
    usage_percent_ = 100.0 * used_kb_ / total_kb_;
  }
}

void MemoryCollector::print_result() const {
  std::cout << "内存信息:" << std::endl;
  std::cout << "  总内存:   " << format_kb(total_kb_) << std::endl;
  std::cout << "  已使用:   " << format_kb(used_kb_) << std::endl;
  std::cout << "  可用:     " << format_kb(available_kb_) << std::endl;
  std::cout << "  使用率:   " << std::fixed << std::setprecision(1)
            << usage_percent_ << "%" << std::endl;
}

// ==================== DiskCollector ====================
void DiskCollector::do_collect() {
  std::ifstream file(DISK_PATH);
  if (!file.is_open()) {
    LOG_ERROR("无法打开 " DISK_PATH);
    return;
  }

  std::ostringstream oss;
  oss << file.rdbuf();
  raw_data_ = oss.str();
  file.close();
}

void DiskCollector::do_parse() {
  disks_.clear();
  std::istringstream iss(raw_data_);
  std::string line;

  while (std::getline(iss, line)) {
    std::stringstream ss(line);
    int major, minor;
    std::string name;
    uint64_t reads_completed, reads_merged, sectors_read, time_reading;
    uint64_t writes_completed, writes_merged, sectors_written, time_writing;

    ss >> major >> minor >> name >> reads_completed >> reads_merged >>
        sectors_read >> time_reading >> writes_completed >> writes_merged >>
        sectors_written >> time_writing;

    // 只记录主要磁盘设备
    if (name.find("loop") == std::string::npos &&
        (name.find("sd") != std::string::npos ||
         name.find("vd") != std::string::npos ||
         name.find("nvme") != std::string::npos)) {

      bool is_partition = false;
      if (name.size() > 2) {
        char last = name.back();
        if (isdigit(last) && name.find("nvme") == std::string::npos) {
          is_partition = true;
        }
        if (name.find("nvme") != std::string::npos &&
            name.find("p") != std::string::npos) {
          is_partition = true;
        }
      }

      if (!is_partition || (name.find("nvme") != std::string::npos &&
                            name.find("p") == std::string::npos)) {
        DiskStats stats;
        stats.name = name;
        stats.reads_completed = reads_completed;
        stats.writes_completed = writes_completed;
        stats.sectors_read = sectors_read;
        stats.sectors_written = sectors_written;
        disks_.push_back(stats);
      }
    }
  }
}

void DiskCollector::print_result() const {
  std::cout << "磁盘 I/O 统计:" << std::endl;
  for (const auto &disk : disks_) {
    std::cout << "  " << disk.name << ":" << std::endl;
    std::cout << "    读取次数: " << disk.reads_completed << std::endl;
    std::cout << "    写入次数: " << disk.writes_completed << std::endl;
    std::cout << "    读取扇区: " << disk.sectors_read << std::endl;
    std::cout << "    写入扇区: " << disk.sectors_written << std::endl;
  }
}

// ==================== NetworkCollector ====================
void NetworkCollector::do_collect() {
  std::ifstream file(NETWORK_PATH);
  if (!file.is_open()) {
    LOG_ERROR("无法打开 " NETWORK_PATH);
    return;
  }

  std::ostringstream oss;
  oss << file.rdbuf();
  raw_data_ = oss.str();
  file.close();
}

void NetworkCollector::do_parse() {
  interfaces_.clear();
  std::istringstream iss(raw_data_);
  std::string line;
  int line_num = 0;

  while (std::getline(iss, line)) {
    line_num++;
    if (line_num <= 2)
      continue; // 跳过头部

    size_t colon_pos = line.find(':');
    if (colon_pos == std::string::npos)
      continue;

    std::string iface_name = line.substr(0, colon_pos);
    iface_name.erase(0, iface_name.find_first_not_of(" \t"));

    std::string stats_str = line.substr(colon_pos + 1);
    std::stringstream ss(stats_str);

    InterfaceStats stats;
    stats.name = iface_name;

    uint64_t dummy;
    ss >> stats.rx_bytes >> stats.rx_packets >> dummy >> dummy >> dummy >>
        dummy >> dummy >> dummy;
    ss >> stats.tx_bytes >> stats.tx_packets;

    interfaces_.push_back(stats);
  }
}

void NetworkCollector::print_result() const {
  std::cout << "网络接口统计:" << std::endl;
  for (const auto &iface : interfaces_) {
    std::cout << "  " << iface.name << ":" << std::endl;
    std::cout << "    接收: " << format_bytes(iface.rx_bytes) << " ("
              << iface.rx_packets << " 包)" << std::endl;
    std::cout << "    发送: " << format_bytes(iface.tx_bytes) << " ("
              << iface.tx_packets << " 包)" << std::endl;
  }
}

// ==================== ProcessCollector ====================
void ProcessCollector::do_collect() {
  // 进程采集直接在 parse 中完成，因为需要遍历目录
  raw_data_.clear();
}

void ProcessCollector::do_parse() {
  DIR *dir = opendir(PROC_PATH);
  if (!dir) {
    LOG_ERROR("无法打开 " PROC_PATH);
    return;
  }

  processes_.clear();
  total_processes_ = 0;
  running_processes_ = 0;

  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    if (entry->d_type == DT_DIR) {
      std::string name = entry->d_name;
      bool is_pid =
          !name.empty() && std::all_of(name.begin(), name.end(), ::isdigit);

      if (is_pid) {
        int pid = std::stoi(name);
        std::string stat_path = std::string(PROC_PATH) + "/" + name + "/stat";
        std::ifstream stat_file(stat_path);

        if (stat_file.is_open()) {
          std::string line;
          if (std::getline(stat_file, line)) {
            ProcessInfo info;
            info.pid = pid;

            size_t start = line.find('(');
            size_t end = line.rfind(')');

            if (start != std::string::npos && end != std::string::npos) {
              info.name = line.substr(start + 1, end - start - 1);

              std::stringstream ss(line.substr(end + 2));
              char state;
              int ppid, pgrp, session, tty_nr, tpgid;
              unsigned flags;
              uint64_t minflt, cminflt, majflt, cmajflt, utime, stime;
              int64_t cutime, cstime, priority, nice, num_threads, itrealvalue;
              uint64_t starttime;

              ss >> state >> ppid >> pgrp >> session >> tty_nr >> tpgid >>
                  flags >> minflt >> cminflt >> majflt >> cmajflt >> utime >>
                  stime >> cutime >> cstime >> priority >> nice >>
                  num_threads >> itrealvalue >> starttime >> info.vsize >>
                  info.rss;

              info.state = state;

              if (state == 'R')
                running_processes_++;

              processes_.push_back(info);
              total_processes_++;
            }
          }
          stat_file.close();
        }
      }
    }
  }
  closedir(dir);

  // 按内存使用排序
  std::sort(
      processes_.begin(), processes_.end(),
      [](const ProcessInfo &a, const ProcessInfo &b) { return a.rss > b.rss; });
}

void ProcessCollector::print_result() const {
  std::cout << "进程统计:" << std::endl;
  std::cout << "  总进程数: " << total_processes_ << std::endl;
  std::cout << "  运行中:   " << running_processes_ << std::endl;

  std::cout << "  Top 5 内存占用进程:" << std::endl;
  int count = 0;
  for (const auto &proc : processes_) {
    if (count >= 5)
      break;
    double rss_mb = proc.rss * 4.0 / 1024.0;
    std::cout << "    [" << proc.pid << "] " << proc.name << " - " << std::fixed
              << std::setprecision(1) << rss_mb << " MB" << std::endl;
    count++;
  }
}

// ==================== SystemCollector ====================
void SystemCollector::do_collect() {
  // 读取 uptime
  std::ifstream uptime_file(UPTIME_PATH);
  if (uptime_file.is_open()) {
    uptime_file >> uptime_seconds_;
    uptime_file.close();
  }

  // 读取 loadavg
  std::ifstream loadavg_file(LOADAVG_PATH);
  if (loadavg_file.is_open()) {
    std::ostringstream oss;
    oss << loadavg_file.rdbuf();
    raw_data_ = oss.str();
    loadavg_file.close();
  }
}

void SystemCollector::do_parse() {
  std::istringstream iss(raw_data_);
  std::string running_str;
  iss >> load_1min_ >> load_5min_ >> load_15min_ >> running_str;

  size_t slash_pos = running_str.find('/');
  if (slash_pos != std::string::npos) {
    running_tasks_ = std::stoi(running_str.substr(0, slash_pos));
    total_tasks_ = std::stoi(running_str.substr(slash_pos + 1));
  }
}

void SystemCollector::print_result() const {
  int days = static_cast<int>(uptime_seconds_ / 86400);
  int hours =
      static_cast<int>((static_cast<int>(uptime_seconds_) % 86400) / 3600);
  int minutes =
      static_cast<int>((static_cast<int>(uptime_seconds_) % 3600) / 60);
  int seconds = static_cast<int>(uptime_seconds_) % 60;

  std::cout << "系统信息:" << std::endl;
  std::cout << "  运行时间: ";
  if (days > 0)
    std::cout << days << " 天 ";
  std::cout << hours << " 小时 " << minutes << " 分钟 " << seconds << " 秒"
            << std::endl;

  std::cout << "  系统负载: " << std::fixed << std::setprecision(2)
            << load_1min_ << " (1分钟), " << load_5min_ << " (5分钟), "
            << load_15min_ << " (15分钟)" << std::endl;

  std::cout << "  任务状态: " << running_tasks_ << " 运行 / " << total_tasks_
            << " 总计" << std::endl;
}