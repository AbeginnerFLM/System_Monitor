#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>

/**
 * 单例模式 (Singleton Pattern)
 * 
 * 目的：确保一个类只有一个实例，并提供全局访问点
 * 
 * 实现要点：
 * 1. 私有构造函数 - 防止外部创建实例
 * 2. 删除拷贝构造和赋值运算符 - 防止复制
 * 3. 静态成员函数返回唯一实例
 * 4. C++11 的 static 局部变量是线程安全的
 */
class Logger {
public:
    // 日志级别枚举
    enum class Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };

    // 获取单例实例 - 这是单例模式的核心
    static Logger& instance() {
        static Logger inst;  // C++11 保证线程安全的懒汉式单例
        return inst;
    }

    // 删除拷贝构造和赋值运算符
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // 设置日志级别
    void set_level(Level level) { current_level_ = level; }

    // 日志输出方法
    void debug(const std::string& msg) {
        log(Level::DEBUG, msg);
    }

    void info(const std::string& msg) {
        log(Level::INFO, msg);
    }

    void warning(const std::string& msg) {
        log(Level::WARNING, msg);
    }

    void error(const std::string& msg) {
        log(Level::ERROR, msg);
    }

private:
    // 私有构造函数 - 单例模式的关键
    Logger() : current_level_(Level::INFO) {}

    Level current_level_;

    void log(Level level, const std::string& msg) {
        if (level < current_level_) return;

        // 获取时间戳
        auto now = std::time(nullptr);
        auto tm = std::localtime(&now);
        
        std::ostringstream oss;
        oss << "[" << std::put_time(tm, "%H:%M:%S") << "] ";
        
        // 级别标签
        switch (level) {
            case Level::DEBUG:   oss << "[DEBUG] "; break;
            case Level::INFO:    oss << "[INFO] "; break;
            case Level::WARNING: oss << "[WARN] "; break;
            case Level::ERROR:   oss << "[ERROR] "; break;
        }
        
        oss << msg;
        
        if (level >= Level::WARNING) {
            std::cerr << oss.str() << std::endl;
        } else {
            std::cout << oss.str() << std::endl;
        }
    }
};

// 便捷宏，简化日志调用
#define LOG_DEBUG(msg) Logger::instance().debug(msg)
#define LOG_INFO(msg)  Logger::instance().info(msg)
#define LOG_WARN(msg)  Logger::instance().warning(msg)
#define LOG_ERROR(msg) Logger::instance().error(msg)

#endif // LOGGER_H
