#include <edgelink/plugin.hpp>
#include "mqtt.client.hpp"

using namespace edgelink;

namespace edgelink::plugins::mqtt {

/*
    {
        "id": "f83cdc7c9c540aa8",
        "type": "mqtt out",
        "z": "7c226c13f2e3b224",
        "name": "",
        "topic": "",
        "qos": "",
        "retain": "",
        "respTopic": "",
        "contentType": "",
        "userProps": "",
        "correl": "",
        "expiry": "",
        "broker": "",
        "x": 870,
        "y": 320,
        "wires": []
    }
*/

class MqttOutNode : public SinkNode {
  public:
    MqttOutNode(FlowNodeID id, const boost::json::object& config, const INodeDescriptor* desc,
                const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : SinkNode(id, desc, move(output_ports), flow, config),
          _mqtt(boost::urls::parse_uri("mqtt://test.mosquitto.org:1883").value()),
          _topic(config.at("topic").as_string()) {
        //
    }

    Awaitable<void> start_async() override {
        co_await _mqtt.async_connect();

        co_return;
    }

    Awaitable<void> stop_async() override { co_return; }

    Awaitable<void> receive_async(std::shared_ptr<Msg> msg) override {
        spdlog::info("MqttOutNode > 收到了消息：\n{0}", msg->to_string());
        co_await _mqtt.publish_async(_topic, msg->to_string(), async_mqtt::qos::at_most_once);
        co_return;
    }

  private:
    MqttClient _mqtt;
    const std::string _topic;
};

RTTR_PLUGIN_REGISTRATION {
    rttr::registration::class_<NodeProvider<MqttOutNode, "mqtt out", NodeKind::SINK>>(
        "edgelink::plugins::modbus::MqttOutNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink::plugins::mqtt