#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <boost/log/trivial.hpp>
#include <boost/format.hpp>
#include <boost/di.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <mqtt/client.h>
#include <nlohmann/json.hpp>

#include "edgelink/edgelink.hpp"
#include "edgelink/logging.hpp"
#include "edgelink/transport/mqtt.hpp"

using namespace std;
using namespace boost;

namespace edgelink {

MqttClient::MqttClient(const EdgeLinkSettings& settings) {
    //
    _address = settings.server_url;
    auto client_id = uuids::to_string(uuids::uuid());
    _mqtt = make_unique<mqtt::client>(_address, client_id);
}

MqttClient::~MqttClient() {
    if (this->is_connected()) {
        this->close();
    }
}

Result<> MqttClient::connect() noexcept {
    spdlog::info("开始连接 MQTT：{0}", _address);

    try {
        mqtt::connect_options connOpts;
        connOpts.set_keep_alive_interval(20);
        connOpts.set_clean_session(true);
        _mqtt->connect(connOpts);
    } catch (const mqtt::exception& ex) {
        return Result<>(std::error_code(ex.get_reason_code(), std::system_category()));
    }

    spdlog::info("MQTT 已连接：{0}", _address);

    return {};
}

void MqttClient::close() noexcept {
    if (this->is_connected()) {
        _mqtt->disconnect();
        spdlog::info("MQTT 连接已断开，主机：", _address);
    }
}

Result<> MqttClient::publish(const std::string_view& topic, mqtt::binary_ref payload, int qos) noexcept {
    try {
        auto msg = mqtt::make_message(topic.data(), payload);
        msg->set_qos(qos);
        _mqtt->publish(msg);
    } catch (const mqtt::exception& ex) {
        return Result<>(std::error_code(ex.get_reason_code(), std::system_category()));
    }
    return {};
}

}; // namespace edgelink