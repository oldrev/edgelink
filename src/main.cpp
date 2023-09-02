#include "pch.hpp"

#include "edgelink/edgelink.hpp"
#include "edgelink/logging.hpp"

using namespace std;
using namespace boost;

// paho.mqtt.cpp 库客户端是线程安全的，可以多个线程同时访问，但是 set_xxx_callback() 设置的回调禁止阻塞

namespace edgelink {

class App {
  public:
    App(std::shared_ptr<nlohmann::json>& json_config, std::shared_ptr<Engine> engine) : _engine(engine) {}

    void run() {
        spdlog::info("正在启动消息引擎...");

        _engine->start();
        _engine->run();
    }

  private:
    std::shared_ptr<Engine> _engine;
};

}; // namespace edgelink

using namespace edgelink;

int main(int argc, char* argv[]) {

    std::cout << "EdgeLink 物联网边缘数据采集系统" << std::endl;
    std::cout << std::endl;

    // 初始化日志系统
    init_logging();

    spdlog::info("日志子系统已初始化");

    std::shared_ptr<::nlohmann::json> json_config = nullptr;
    try {
        std::ifstream config_file("./edgelink-conf.json");
        json_config = std::make_shared<::nlohmann::json>(::nlohmann::json::parse(config_file, nullptr, true, true));
    } catch (std::exception& ex) {
        spdlog::critical("读取配置文件错误：{0}", ex.what());
        return -1;
    } catch (...) {
        spdlog::critical("未知错误");
        return -1;
    }

    const auto injector = di::make_injector(                   //
        di::bind<>().to(json_config),                          //
        di::bind<App>().in(di::singleton),                     // App
        di::bind<Engine>().in(di::singleton),                  // Engine
        di::bind<IRegistry>().to<Registry>().in(di::singleton) // Engine
    );

    auto app = injector.create<App>();

    // 启动主程序
    try {
        app.run();
    } catch (std::exception& ex) {
        spdlog::critical("程序异常！错误消息：{0}", ex.what());
        return -1;
    }
}
