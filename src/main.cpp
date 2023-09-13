#include "version-config.h"
#include "edgelink/edgelink.hpp"
#include "logging.hpp"

#include "app.hpp"

#include "flows/flow.hpp"
#include "flows/flow-factory.hpp"

using namespace boost;
namespace this_coro = boost::asio::this_coro;

// paho.mqtt.cpp 库客户端是线程安全的，可以多个线程同时访问，但是 set_xxx_callback() 设置的回调禁止阻塞

using namespace edgelink;

int main(int argc, char* argv[]) {

    std::cout << "EdgeLink 物联网边缘数据采集系统" << std::endl;
    std::cout << "版本：" << EDGELINK_VERSION << "\t" << "REV: " << GIT_REVISION << std::endl;
    std::cout << std::endl;

    // 初始化日志系统
    init_logging();

    SPDLOG_INFO("日志子系统已初始化");

    std::shared_ptr<boost::json::object> json_config = nullptr;
    try {
        std::ifstream config_file("./edgelink-conf.json");
        auto parsed =
            boost::json::parse(config_file, {}, {.allow_comments = true, .allow_trailing_commas = true}).as_object();
        json_config = std::make_shared<boost::json::object>(std::move(parsed));
    } catch (std::exception& ex) {
        SPDLOG_CRITICAL("读取配置文件错误：{0}", ex.what());
        return -1;
    } catch (...) {
        SPDLOG_CRITICAL("未知错误");
        return -1;
    }

    EdgeLinkConfig el_config{
        .flows_json_path = "./flows.json",
    };

    const auto injector = di::make_injector(
        di::bind<>().to(el_config),                                                           // EdgeLinkConfig
        di::bind<>().to(json_config),                                                         // App
        di::bind<App>().in(di::singleton),                                                    // App
        di::bind<IEngine>().to<Engine>().in(di::singleton),                                   // Engine
        di::bind<IRegistry>().to<Registry>().in(di::singleton),                               // Registry
        di::bind<IFlowFactory>().to<edgelink::flows::FlowFactory>().in(di::singleton) // Engine
    );

    auto app = injector.create<App>();

    // 启动主程序
    try {
        auto nconcurrency = std::thread::hardware_concurrency() + 1;
        asio::io_context io_context(nconcurrency);
        SPDLOG_INFO("系统并发数量：{}", nconcurrency);

        asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto) {
            io_context.stop();
            // std::terminate();
        });

        asio::co_spawn(io_context, app.run_async(), asio::detached);

        io_context.run();
        SPDLOG_INFO("系统协程系统已停止，开始进行清理...");
        spdlog::shutdown();
    } catch (std::exception& ex) {
        SPDLOG_CRITICAL("程序异常！错误消息：{0}", ex.what());
        spdlog::shutdown();
        return -1;
    }
}
