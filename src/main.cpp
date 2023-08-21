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
#include "edgelink/edgelink.hpp"
#include "edgelink/mqtt.hpp"

using namespace std;
using namespace boost;

// paho.mqtt.cpp 库客户端是线程安全的，可以多个线程同时访问，但是 set_xxx_callback() 设置的回调禁止阻塞

namespace edgelink {

struct IGreeter {
    virtual ~IGreeter() noexcept = default;
    virtual void say() = 0;
};

class GreeterImpl : public virtual IGreeter {
  public:
    virtual void say() override {
        //
        cout << "Hello!\n";
    }

    virtual ~GreeterImpl() {
        //
        cout << "i'm dead!\n";
    }
};

class Example {
  public:
    Example(std::shared_ptr<IGreeter> greeter) {
        //
        _greeter = greeter;
    }

    void greet() { _greeter->say(); }

  private:
    std::shared_ptr<IGreeter> _greeter;
};

const string CLIENT_ID("33f1c750-01a6-4a26-9057-6a5adf0f80f5");
const int QOS = 2;

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

        /*
            cout << "Initialzing..." << endl;
            mqtt::client client(settings->server_url, settings->device_id);

            user_callback cb;
            client.set_callback(cb);

            mqtt::connect_options connOpts;
            connOpts.set_keep_alive_interval(20);
            connOpts.set_clean_session(true);
            cout << "...OK" << endl;

            try {
                cout << "\nConnecting..." << endl;
                client.connect(connOpts);
                cout << "...OK" << endl;

                // First use a message pointer.

                for (int i = 0; i < 1000; i++) {
                    cout << "\nSending message..." << endl;
                    auto pubmsg = mqtt::make_message(TOPIC, "Hello World,This is a message...");
                    pubmsg->set_qos(QOS);
                    client.publish(pubmsg);
                    cout << "...OK" << endl;
                    std::this_thread::sleep_for(1000ms);
                }

                // Disconnect
                cout << "\nDisconnecting..." << endl;
                client.disconnect();
                cout << "...OK" << endl;
            } catch (const mqtt::persistence_exception& exc) {
                cerr << "Persistence Error: " << exc.what() << " [" << exc.get_reason_code() << "]" << endl;
                return 1;
            } catch (const mqtt::exception& exc) {
                cerr << exc.what() << endl;
                return 1;
            }
            */

        cout << "\nExiting" << endl;

        return {};
    }

  private:
    const EdgeLinkSettings& _settings;
    std::shared_ptr<MqttClient> _client;
};

}; // namespace edgelink

using namespace edgelink;

int main(int argc, char* argv[]) {

    // 初始化日志系统
    init_logging();

    BOOST_LOG_TRIVIAL(info) << "日志子系统已初始化";

    auto settings_result = load_settings();
    if (settings_result.has_error()) {
        BOOST_LOG_TRIVIAL(error) << "读取配置文件错误";
        return -1;
    }
    di::aux::owner<EdgeLinkSettings*> settings_owner{settings_result.value()};

    const auto injector = di::make_injector(                           //
        di::bind<App>().in(di::singleton),                       // App
        di::bind<EdgeLinkSettings>().to(*settings_owner),        // 系统配置
        di::bind<MqttClient>().in(di::singleton),                //
        di::bind<IGreeter>().to<GreeterImpl>().in(di::singleton) // 测试用
    );

    auto app = injector.create<App>();

    auto result = app.run();

    if (result.has_error()) {
        return -1;
    } else {
        return 0;
    }
}
