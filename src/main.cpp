#include <fstream>
#include <iostream>
#include <map>
#include <span>
#include <string>
#include <thread>
#include <vector>

#include <boost/di.hpp>

#include <mqtt/client.h>
#include <nlohmann/json.hpp>

#include "edgelink/edgelink.hpp"
#include "edgelink/engine.hpp"
#include "edgelink/logging.hpp"
#include "edgelink/transport/mqtt.hpp"

using namespace std;
using namespace boost;

// paho.mqtt.cpp 库客户端是线程安全的，可以多个线程同时访问，但是 set_xxx_callback() 设置的回调禁止阻塞

namespace edgelink {

EdgeLinkSettings* load_settings() {
    std::ifstream config_file("./edgelink-conf.json");
    auto json_config = nlohmann::json::parse(config_file);
    // auto settings = std::make_shared<EdgeLinkSettings>();
    auto settings = new EdgeLinkSettings;
    settings->project_id = json_config["projectID"];
    settings->device_id = json_config["deviceID"];
    settings->server_url = json_config["serverUrl"];
    return settings;
}

class App {
  public:
    App(const EdgeLinkSettings& settings) : _settings(settings) {}

    void run() { _engine.run(); }

  private:
    const EdgeLinkSettings& _settings;
    std::shared_ptr<MqttClient> _client;
    Engine _engine;
};

}; // namespace edgelink

using namespace edgelink;

int main(int argc, char* argv[]) {

    std::cout << "EdgeLink 物联网边缘数据采集系统" << std::endl;
    std::cout << std::endl;

    // 初始化日志系统
    init_logging();

    spdlog::info("日志子系统已初始化");

    EdgeLinkSettings* settings = nullptr;
    try {
        settings = load_settings();
    } catch (std::exception& ex) {
        spdlog::critical("读取配置文件错误：{0}", ex.what());
        return -1;
    }

    di::aux::owner<EdgeLinkSettings*> settings_owner{settings};

    const auto injector = di::make_injector(              //
        di::bind<App>().in(di::singleton),                // App
        di::bind<EdgeLinkSettings>().to(*settings_owner), // 注册系统配置
        di::bind<MqttClient>().in(di::singleton)          // 注册 MQTT 客户端单体
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
