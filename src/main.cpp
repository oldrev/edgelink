#include <fstream>
#include <iostream>
#include <map>
#include <span>
#include <string>
#include <thread>
#include <vector>

#include <boost/format.hpp>
#include <boost/di.hpp>

#include <mqtt/client.h>
#include <nlohmann/json.hpp>

#include "edgelink/logging.hpp"
#include "edgelink/engine.hpp"
#include "edgelink/edgelink.hpp"
#include "edgelink/transport/mqtt.hpp"

using namespace std;
using namespace boost;

// paho.mqtt.cpp 库客户端是线程安全的，可以多个线程同时访问，但是 set_xxx_callback() 设置的回调禁止阻塞

namespace edgelink {

class user_callback : public virtual mqtt::callback {
    void connection_lost(const string& cause) override {
        cout << "\nConnection lost" << endl;
        if (!cause.empty())
            cout << "\tcause: " << cause << endl;
    }

    void delivery_complete(mqtt::delivery_token_ptr tok) override {
        cout << "\n\t[Delivery complete for token: " << (tok ? tok->get_message_id() : -1) << "]" << endl;
    }

  public:
};

Result<EdgeLinkSettings*> load_settings() {
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
    App(const EdgeLinkSettings& settings, std::shared_ptr<MqttClient> client) : _settings(settings) {
        _client = client;
    }

    Result<> run() {

        TRY(_client->connect());

        string TOPIC("/" + _settings.project_id + "/data/test1");

        TRY(_client->publish(TOPIC, "Hello!!! Love from EdgeLink!", 1));

        cout << "\nExiting" << endl;

        return {};
    }

  private:
    const EdgeLinkSettings& _settings;
    std::shared_ptr<MqttClient> _client;
    Engine _engine;
};

}; // namespace edgelink

using namespace edgelink;

int main(int argc, char* argv[]) {

    // 初始化日志系统
    init_logging();

    spdlog::info("EdgeLink 物联网边缘数据采集系统");
    spdlog::info("日志子系统已初始化");

    auto settings_result = load_settings();
    if (settings_result.has_error()) {
        spdlog::critical("读取配置文件错误");
        return -1;
    }
    di::aux::owner<EdgeLinkSettings*> settings_owner{settings_result.value()};

    const auto injector = di::make_injector(              //
        di::bind<App>().in(di::singleton),                // App
        di::bind<EdgeLinkSettings>().to(*settings_owner), // 注册系统配置
        di::bind<MqttClient>().in(di::singleton)          // 注册 MQTT 客户端单体
    );

    auto app = injector.create<App>();

    // 启动主程序
    auto result = app.run();

    if (result.has_error()) {
        return -1;
    } else {
        return 0;
    }
}
