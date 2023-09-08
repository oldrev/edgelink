#include <edgelink/plugin.hpp>
#include "mqtt.client.hpp"
#include "mqtt.hpp"

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
    MqttOutNode(const std::string_view id, const boost::json::object& config, const INodeDescriptor* desc,
                const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : SinkNode(id, desc, move(output_ports), flow, config), _mqtt_broker_node_id(config.at("broker").as_string()) {
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

            if (auto retain_value = config.if_contains("retain")) {
                const std::string_view retain_str = retain_value->as_string();
                if (retain_str == "true") {
                    _node_retail = true;
                } else if (retain_str == "false") {
                    _node_retail = false;
                } else {
                    // 保持为空
                }
            }
        } catch (std::exception& ex) {
            spdlog::error("加载 MQTT Out 节点配置发生错误：{0}", ex.what());
            throw ex;
        }
    }

    Awaitable<void> start_async() override { co_return; }

    Awaitable<void> stop_async() override { co_return; }

    Awaitable<void> receive_async(std::shared_ptr<Msg> msg) override {

        if (msg->data().contains("payload")) {

            auto topic = _node_topic.has_value() ? std::string_view(*_node_topic)
                                                 : std::string_view(msg->data().at("topic").as_string());
            auto qos = _node_qos.has_value() ? *_node_qos : async_mqtt::qos(msg->data().at("qos").to_number<int>());

            auto json_payload_value = msg->data().at("payload");

            auto mqtt_node = this->flow()->engine()->get_global_node(_mqtt_broker_node_id);
            auto mqtt = dynamic_cast<IMqttBrokerEndpoint*>(mqtt_node);

            if (json_payload_value.is_string()) { // 是字符串就原样发送
                co_await mqtt->async_publish_string(topic, json_payload_value.as_string(), qos);
            } else if (json_payload_value.is_array()) { // 是数组就假定要发送的是字节数组
                // 注意不能直接发，这里是 boost::array，需要转换 buffer
                auto json_array = json_payload_value.as_array();
                std::vector<char> bytes(json_array.size());
                for (size_t i = 0; i < json_array.size(); i++) {
                    auto v = json_array.at(i);
                    bytes[i] = v.to_number<char>();
                }
                async_mqtt::buffer buffer(bytes.begin(), bytes.end());
                co_await mqtt->async_publish(topic, buffer, qos);
            } else if (json_payload_value.is_object()) { // 如果是对象就转换为 JSON 字符串发送
                auto payload_text = std::move(boost::json::serialize(msg->data().at("payload")));
                co_await mqtt->async_publish_string(topic, payload_text, qos);
            } else {
                throw InvalidDataException(
                    fmt::format("MQTT 输出节点不支持负载：'{0}'", boost::json::serialize(json_payload_value)));
            }
        }
        co_return;
    }

  private:
    std::string _mqtt_broker_node_id;
    std::optional<std::string> _node_topic;
    std::optional<async_mqtt::qos> _node_qos;
    std::optional<bool> _node_retail;
};

RTTR_PLUGIN_REGISTRATION {
    rttr::registration::class_<FlowNodeProvider<MqttOutNode, "mqtt out", NodeKind::SINK>>(
        "edgelink::plugins::mqtt::MqttOutNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink::plugins::mqtt