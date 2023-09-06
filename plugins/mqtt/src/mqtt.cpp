
#include "mqtt.hpp"

using namespace boost;

namespace edgelink::plugins::mqtt {

MqttClient::MqttClient(const boost::json::object& json_config) {
    //
    _address = std::move(std::string(json_config.at("test").as_string()));
    auto client_id = uuids::to_string(uuids::uuid());
}

MqttClient::~MqttClient() {
    if (this->is_connected()) {
        this->close();
    }
}

void MqttClient::connect() {
    spdlog::info("开始连接 MQTT：{0}", _address);
    spdlog::info("MQTT 已连接：{0}", _address);
}

void MqttClient::close() noexcept { spdlog::info("MQTT 连接已断开，主机：", _address); }

void MqttClient::publish(const std::string_view& topic, const void* buf, int qos) {}

}; // namespace edgelink::plugins::mqtt