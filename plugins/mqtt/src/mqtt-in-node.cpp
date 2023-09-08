#include <edgelink/plugin.hpp>
#include "mqtt.client.hpp"
#include "mqtt.hpp"

using namespace edgelink;

namespace edgelink::plugins::mqtt {

/*
    {
        "id": "72be09264971c5ec",
        "type": "mqtt in",
        "z": "73e0fcd142fc5256",
        "name": "",
        "topic": "/yncic-dangerous/data/test1",
        "qos": "0",
        "datatype": "auto-detect",
        "broker": "3c39cf63714c26a4",
        "nl": false,
        "rap": true,
        "rh": 0,
        "inputs": 0,
        "x": 360,
        "y": 380,
        "wires": [
            [
                "adde85bf75a42c9c"
            ]
        ]
    },
*/

class MqttInNode : public SourceNode {
  public:
    MqttInNode(const std::string_view id, const boost::json::object& config, const INodeDescriptor* desc,
               const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : SourceNode(id, desc, move(output_ports), flow, config), _mqtt_broker_node_id(config.at("broker").as_string()) {
        try {
            //
            if (auto topic_value = config.if_contains("topic")) {
                const std::string_view topic_str = topic_value->as_string();
                if (!topic_str.empty()) {
                    _node_topic = std::string(topic_str);
                } else {
                    // 保持为空
                }
            }

            if (auto qos_value = config.if_contains("qos")) {
                const std::string_view qos_str = qos_value->as_string();
                if (qos_str == "0") {
                    _node_qos = async_mqtt::qos::at_most_once;
                } else if (qos_str == "1") {
                    _node_qos = async_mqtt::qos::at_least_once;
                } else if (qos_str == "2") {
                    _node_qos = async_mqtt::qos::exactly_once;
                } else {
                    // 不动
                }
            }

        } catch (std::exception& ex) {
            spdlog::error("加载 MQTT In 节点配置发生错误：{0}", ex.what());
            throw ex;
        }
    }

    Awaitable<void> process_async(std::stop_token& stoken) override {
        //

        co_return;
    }

  private:
    std::string _mqtt_broker_node_id;
    std::optional<std::string> _node_topic;
    std::optional<async_mqtt::qos> _node_qos;
};

RTTR_PLUGIN_REGISTRATION {
    rttr::registration::class_<FlowNodeProvider<MqttInNode, "mqtt in", NodeKind::SOURCE>>(
        "edgelink::plugins::modbus::MqttInNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};
}; // namespace edgelink::plugins::mqtt