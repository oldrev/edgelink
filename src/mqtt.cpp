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
#include "edgelink/mqtt.hpp"

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

void MqttClient::connect() {
    BOOST_LOG_TRIVIAL(info) << "开始连接 MQTT" << _address;

    mqtt::connect_options connOpts;
    connOpts.set_keep_alive_interval(20);
    connOpts.set_clean_session(true);

    _mqtt->connect(connOpts);

    BOOST_LOG_TRIVIAL(info) << "MQTT 已连接：" << _address;
}

void MqttClient::close() noexcept {
    if (this->is_connected()) {
        _mqtt->disconnect();
        BOOST_LOG_TRIVIAL(info) << "MQTT 连接已断开，主机：" << _address;
    }
}

void MqttClient::publish(const std::string_view& topic, mqtt::binary_ref payload, int qos) {
    auto msg = mqtt::make_message(topic.data(), payload);
    msg->set_qos(qos);
    _mqtt->publish(msg);
}

}; // namespace edgelink