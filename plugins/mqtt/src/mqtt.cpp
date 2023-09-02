#include "pch.hpp"

#include <edgelink/plugin.hpp>

#include "mqtt.hpp"

using namespace std;
using namespace boost;

namespace edgelink::plugins::mqtt {

MqttClient::MqttClient(const ::nlohmann::json& json_config) {
    //
    _address = json_config["test"];
    auto client_id = uuids::to_string(uuids::uuid());
    _mqtt = make_unique<::mqtt::client>(_address, client_id);
}

MqttClient::~MqttClient() {
    if (this->is_connected()) {
        this->close();
    }
}

void MqttClient::connect() {
    spdlog::info("开始连接 MQTT：{0}", _address);

    ::mqtt::connect_options connOpts;
    connOpts.set_keep_alive_interval(20);
    connOpts.set_clean_session(true);
    _mqtt->connect(connOpts);

    spdlog::info("MQTT 已连接：{0}", _address);
}

void MqttClient::close() noexcept {
    if (this->is_connected()) {
        _mqtt->disconnect();
        spdlog::info("MQTT 连接已断开，主机：", _address);
    }
}

void MqttClient::publish(const std::string_view& topic, ::mqtt::binary_ref payload, int qos) {
    auto msg = ::mqtt::make_message(topic.data(), payload);
    msg->set_qos(qos);
    _mqtt->publish(msg);
}

}; // namespace edgelink::plugins::mqtt