#ifndef COLLECTORS_H
#define COLLECTORS_H

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cstdint>
#include <dirent.h>

// 数据源路径
#define CPU_PATH "/proc/stat"
#define MEMORY_PATH "/proc/meminfo"
#define NETWORK_PATH "/proc/net/dev"
#define DISK_PATH "/proc/diskstats"
#define UPTIME_PATH "/proc/uptime"
#define LOADAVG_PATH "/proc/loadavg"
#define PROC_PATH "/proc"

/**
 * 模板方法模式 (Template Method Pattern)
 * 
 * 目的：定义算法的骨架，将某些步骤延迟到子类实现
 * 
 * 结构：
 * - update() : 模板方法，定义算法骨架（final 防止子类覆盖）
 * - do_collect() : 抽象方法，子类必须实现
 * - do_parse() : 抽象方法，子类必须实现  
 * - do_calculate() : 钩子方法，子类可选覆盖，有默认空实现
 */
class Collector {
public:
    virtual ~Collector() = default;

    // 模板方法 - 定义采集流程的骨架
    // 非虚函数，子类无法覆盖
    void update() {
        do_collect();    // 步骤1: 采集原始数据
        do_parse();      // 步骤2: 解析数据
        do_calculate();  // 步骤3: 计算结果（可选）
    }

    // 获取采集器名称 - 用于工厂模式注册
    virtual std::string get_name() const = 0;

    // 打印结果
    virtual void print_result() const = 0;

protected:
    std::string raw_data_;  // 原始数据

    // 抽象方法 - 子类必须实现
    virtual void do_collect() = 0;
    virtual void do_parse() = 0;

    // 钩子方法 - 子类可选覆盖
    virtual void do_calculate() {}
};

// ==================== CPU 采集器 ====================
class CPUCollector : public Collector {
public:
    std::string get_name() const override { return "cpu"; }
    void print_result() const override;
    double get_usage() const { return usage_percent_; }

protected:
    void do_collect() override;
    void do_parse() override;
    void do_calculate() override;

private:
    unsigned long long prev_idle_ = 0;
    unsigned long long prev_total_ = 0;
    unsigned long long curr_idle_ = 0;
    unsigned long long curr_total_ = 0;
    double usage_percent_ = 0.0;
};

// ==================== 内存采集器 ====================
class MemoryCollector : public Collector {
public:
    std::string get_name() const override { return "memory"; }
    void print_result() const override;
    double get_usage() const { return usage_percent_; }

protected:
    void do_collect() override;
    void do_parse() override;
    void do_calculate() override;

private:
    uint64_t total_kb_ = 0;
    uint64_t free_kb_ = 0;
    uint64_t available_kb_ = 0;
    uint64_t buffers_kb_ = 0;
    uint64_t cached_kb_ = 0;
    uint64_t used_kb_ = 0;
    double usage_percent_ = 0.0;
};

// ==================== 磁盘采集器 ====================
class DiskCollector : public Collector {
public:
    std::string get_name() const override { return "disk"; }
    void print_result() const override;

protected:
    void do_collect() override;
    void do_parse() override;

private:
    struct DiskStats {
        std::string name;
        uint64_t reads_completed = 0;
        uint64_t writes_completed = 0;
        uint64_t sectors_read = 0;
        uint64_t sectors_written = 0;
    };
    std::vector<DiskStats> disks_;
};

// ==================== 网络采集器 ====================
class NetworkCollector : public Collector {
public:
    std::string get_name() const override { return "network"; }
    void print_result() const override;

protected:
    void do_collect() override;
    void do_parse() override;

private:
    struct InterfaceStats {
        std::string name;
        uint64_t rx_bytes = 0;
        uint64_t tx_bytes = 0;
        uint64_t rx_packets = 0;
        uint64_t tx_packets = 0;
    };
    std::vector<InterfaceStats> interfaces_;
};

// ==================== 进程采集器 ====================
class ProcessCollector : public Collector {
public:
    std::string get_name() const override { return "process"; }
    void print_result() const override;

protected:
    void do_collect() override;
    void do_parse() override;

private:
    struct ProcessInfo {
        int pid;
        std::string name;
        char state;
        uint64_t vsize;
        int64_t rss;
    };
    std::vector<ProcessInfo> processes_;
    int total_processes_ = 0;
    int running_processes_ = 0;
};

// ==================== 系统信息采集器 ====================
class SystemCollector : public Collector {
public:
    std::string get_name() const override { return "system"; }
    void print_result() const override;

protected:
    void do_collect() override;
    void do_parse() override;

private:
    double uptime_seconds_ = 0.0;
    double load_1min_ = 0.0;
    double load_5min_ = 0.0;
    double load_15min_ = 0.0;
    int running_tasks_ = 0;
    int total_tasks_ = 0;
};

#endif // COLLECTORS_H