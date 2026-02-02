#ifndef COLLECTOR_FACTORY_H
#define COLLECTOR_FACTORY_H

#include "Collectors.h"
#include <functional>
#include <memory>
#include <vector>

/**
 * 工厂模式 (Factory Pattern)
 *
 * 目的：将对象创建逻辑集中管理，客户端不需要知道具体类名
 *
 * 优点：
 * 1. 解耦 - 客户端只依赖接口，不依赖具体类
 * 2. 扩展性 - 添加新采集器只需注册，无需修改客户端代码
 * 3. 集中管理 - 所有创建逻辑在一处
 *
 * 改进：使用 vector 替代 map 存储注册信息，利用静态初始化的顺序（在单文件中）
 * 自动决定显示顺序，无需硬编码 order 数组。
 */
class CollectorFactory {
public:
  // 创建函数类型
  using CreatorFunc = std::function<std::unique_ptr<Collector>()>;

  // 获取工厂单例
  static CollectorFactory &instance() {
    static CollectorFactory factory;
    return factory;
  }

  // 注册采集器创建函数 (不再需要名字)
  void register_collector(CreatorFunc creator) {
    creators_.emplace_back(std::move(creator));
  }

  // 创建所有已注册的采集器
  std::vector<std::unique_ptr<Collector>> create_all() const {
    std::vector<std::unique_ptr<Collector>> collectors;
    // 直接按注册顺序创建
    for (const auto &creator : creators_) {
      collectors.push_back(creator());
    }
    return collectors;
  }

  // 禁止拷贝和赋值
  CollectorFactory(const CollectorFactory &creator) = delete;
  CollectorFactory &operator=(const CollectorFactory &creator) = delete;

private:
  CollectorFactory() = default;
  // 使用 vector 保持注册顺序
  std::vector<CreatorFunc> creators_;
};

/**
 * 自注册辅助模板类
 *
 * 利用静态变量在程序启动时自动注册采集器
 * 这样添加新采集器时，只需要在 cpp 文件中添加一行注册代码
 */
template <typename T> class CollectorRegistrar {
public:
  // 构造函数不需要名字了
  CollectorRegistrar() {
    CollectorFactory::instance().register_collector(
        []() { return std::make_unique<T>(); });
  }
};

// static
// 关键字只能修饰变量声明，不能修饰函数调用表达式。所以要借助CollectorRegistrar类

// 便捷宏 - 彻底移除名字参数
#define REGISTER_COLLECTOR(type)                                               \
  static CollectorRegistrar<type> registrar_##type

#endif // COLLECTOR_FACTORY_H
