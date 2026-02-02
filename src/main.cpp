#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <memory>

#include "Collectors.h"
#include "CollectorFactory.h"
#include "Logger.h"

// ANSI 转义码 - 用于清屏和光标控制
#define CLEAR_SCREEN "\033[2J\033[H"
#define BOLD "\033[1m"
#define RESET "\033[0m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define CYAN "\033[36m"

void print_header() {
    std::cout << BOLD << CYAN;
    std::cout << "╔══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║               Linux 系统资源监控器                         ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════╝" << RESET << std::endl;
    std::cout << std::endl;
}

void print_separator() {
    std::cout << YELLOW << "──────────────────────────────────────────────────────────────" << RESET << std::endl;
}

int main() {
    // 设置日志级别（单例模式示例）
    Logger::instance().set_level(Logger::Level::WARNING);

    // 1. 创建定时器 fd
    int tfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (tfd == -1) {
        LOG_ERROR(std::string("创建 timerfd 失败: ") + strerror(errno));
        return 1;
    }

    // 2. 设置定时参数（每 1 秒触发一次）
    struct itimerspec ts;
    ts.it_interval.tv_sec = 1;   // 周期：1秒
    ts.it_interval.tv_nsec = 0;
    ts.it_value.tv_sec = 0;      // 第一次启动：立即开始
    ts.it_value.tv_nsec = 1;     // 设置为非零值以启动定时器
    
    if (timerfd_settime(tfd, 0, &ts, NULL) == -1) {
        LOG_ERROR(std::string("设置 timerfd 失败: ") + strerror(errno));
        close(tfd);
        return 1;
    }

    // 3. 将 tfd 加入 epoll
    int epfd = epoll_create1(0);
    if (epfd == -1) {
        LOG_ERROR(std::string("创建 epoll 失败: ") + strerror(errno));
        close(tfd);
        return 1;
    }
    
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = tfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, tfd, &ev) == -1) {
        LOG_ERROR(std::string("epoll_ctl 失败: ") + strerror(errno));
        close(tfd);
        close(epfd);
        return 1;
    }

    // ========================================
    // 工厂模式：使用工厂创建所有采集器
    // 客户端不需要知道具体类名！
    // ========================================
    auto collectors = CollectorFactory::instance().create_all();

    // 首次采集数据（模板方法模式：调用 update()）
    for (auto& collector : collectors) {
        collector->update();  // 模板方法：自动调用 do_collect -> do_parse -> do_calculate
    }

    std::cout << "系统监控器已启动，按 Ctrl+C 退出..." << std::endl;
    LOG_INFO("系统监控器已启动");

    // 4. 事件循环
    struct epoll_event events[1];
    while (true) {
        int nfds = epoll_wait(epfd, events, 1, -1);
        
        if (nfds == -1) {
            LOG_ERROR(std::string("epoll_wait 失败: ") + strerror(errno));
            break;
        }
        
        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == tfd) {
                // 定时器触发
                uint64_t expirations;
                ssize_t s = read(tfd, &expirations, sizeof(expirations));
                if (s != sizeof(expirations)) {
                    LOG_ERROR("读取 timerfd 失败");
                    continue;
                }
                
                // 清屏并打印头部
                std::cout << CLEAR_SCREEN;
                print_header();
                
                // 多态遍历 + 模板方法模式
                for (size_t i = 0; i < collectors.size(); ++i) {
                    collectors[i]->update();       // 模板方法
                    collectors[i]->print_result(); // 多态调用
                    if (i < collectors.size() - 1) {
                        print_separator();
                    }
                }
                
                std::cout << std::endl;
                std::cout << GREEN << "[每秒自动刷新 | Ctrl+C 退出]" << RESET << std::endl;
            }
        }
    }

    // 清理
    close(tfd);
    close(epfd);
    
    return 0;
}