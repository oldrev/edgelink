#include <edgelink/plugin.hpp>
#include "mqtt.hpp"

using namespace edgelink;

namespace edgelink::plugins::mqtt {

using namespace boost;
namespace asio = boost::asio;
namespace am = async_mqtt;
namespace this_coro = boost::asio::this_coro;
/*
    {
        "id": "3c39cf63714c26a4",
        "type": "mqtt-broker",
        "name": "",
        "broker": "test.mosquitto.org",
        "port": "1883",
        "clientid": "1883",
        "autoConnect": true,
        "usetls": false,
        "protocolVersion": "4",
        "keepalive": "60",
        "cleansession": true,
        "birthTopic": "",
        "birthQos": "0",
        "birthPayload": "",
        "birthMsg": {},
        "closeTopic": "",
        "closeQos": "0",
        "closePayload": "",
        "closeMsg": {},
        "willTopic": "",
        "willQos": "0",
        "willPayload": "",
        "willMsg": {},
        "userProps": "",
        "sessionExpiry": ""
    }
*/

class MqttBrokerNode : public EndpointNode,
                       public std::enable_shared_from_this<MqttBrokerNode>,
                       public IMqttBrokerEndpoint {
  public:
    MqttBrokerNode(const std::string_view id, const boost::json::object& config, const INodeDescriptor* desc,
                   const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : EndpointNode(id, desc, std::move(output_ports), flow, config, config.at("broker").as_string(),
                       boost::lexical_cast<uint16_t>(config.at("port").as_string().c_str())) {
        //
    }

    const std::string_view host() const noexcept override { return _host; }

    uint16_t address() const noexcept override { return _port; }

    Awaitable<void> start_async() override { co_return; }

    Awaitable<void> stop_async() override { co_return; }

    Awaitable<void> receive_async(std::shared_ptr<Msg> msg) override { co_return; }

    bool is_connected() const override { return _endpoint && _endpoint->next_layer().is_open(); }

    Awaitable<void> async_publish(const std::string_view topic, const async_mqtt::buffer& payload_buffer,
                                  async_mqtt::qos qos) override {
        auto topic_buffer = am::allocate_buffer(topic);
        auto pid = co_await _endpoint->acquire_unique_packet_id(asio::use_awaitable);
        // Send MQTT PUBLISH
        auto se = co_await _endpoint->send(am::v3_1_1::publish_packet{*pid, topic_buffer, payload_buffer, qos},
                                           asio::use_awaitable);
        if (se) {
            spdlog::error("MQTT PUBLISH send error: {0}", se.what());
            co_return;
        }
        // Recv MQTT PUBLISH and PUBACK (order depends on broker)
        am::packet_variant pv = co_await _endpoint->recv(asio::use_awaitable);
        if (pv) {
            pv.visit(am::overload{[&](am::v3_1_1::puback_packet const& p) {
                                      //
                                      // spdlog::info("MQTT PUBACK recv pid: {0}", p.packet_id());
                                      return;
                                  },
                                  [](auto const&) {}});
        } else {
            spdlog::error("MQTT recv error: {0}", pv.get<am::system_error>().what());
        }
        co_return;
    }

    Awaitable<void> async_publish_string(const std::string_view topic, const std::string_view payload,
                                         async_mqtt::qos qos) override {
        auto topic_buffer = am::allocate_buffer(topic);
        auto payload_buffer = am::allocate_buffer(payload);
        co_await this->async_publish(topic_buffer, payload_buffer, qos);
        co_return;
    }

  private:
    Awaitable<void> async_connect() {
        spdlog::info("开始连接 MQTT：{0}:{1}", _host, _port);

        auto exe = co_await this_coro::executor;

        {
            auto amep = new Endpoint{am::protocol_version::v3_1_1, exe};
            _endpoint = std::move(std::unique_ptr<Endpoint>(amep));
        }

        // asio::ip::tcp::socket resolve_sock{exe};
        asio::ip::tcp::resolver resolver(exe);

        // Resolve hostname
        spdlog::info("MqttBrokerNode > 解析地址");

        auto eps = co_await resolver.async_resolve(_host, std::to_string(_port), asio::use_awaitable);

        // Layer
        // am::stream -> TCP

        // Underlying TCP connect
        spdlog::info("MqttBrokerNode > socket 开始连接");
        co_await asio::async_connect(_endpoint->next_layer(), eps, asio::use_awaitable);

        // Send MQTT CONNECT
        if (auto se = co_await _endpoint->send(
                am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    am::allocate_buffer("cid1"),
                    am::nullopt, // will
                    am::nullopt, // username set like am::allocate_buffer("user1"),
                    am::nullopt  // password set like am::allocate_buffer("pass1")
                },
                asio::use_awaitable)) {
            co_return;
        }

        // Recv MQTT CONNACK
        if (am::packet_variant pv = co_await _endpoint->recv(asio::use_awaitable)) {
            auto cb = am::overload{[&](am::v3_1_1::connack_packet const& p) {
                                       // std::cout << "MQTT CONNACK recv" << " sp:" << p.session_present() <<
                                       // std::endl; spdlog::info("MqttClient > 收到连接相应");
                                   },
                                   [](auto const&) {}};
            pv.visit(cb);
        } else {
            spdlog::error("MqttClient > CONNACK 收到错误：{0}", pv.get<am::system_error>().what());
            co_return;
        }

        spdlog::info("MQTT 已连接：{0}:{1}", _host, _port);
    }

    /// @brief 关闭连接
    /// @return
    Awaitable<void> async_close() noexcept {
        co_await _endpoint->close(asio::use_awaitable);
        spdlog::info("MQTT 连接已断开，主机：{0}:{1}", _host, _port);
    }

  private:
    std::string _host;
    uint16_t _port;
    std::unique_ptr<Endpoint> _endpoint;
};

RTTR_PLUGIN_REGISTRATION {
    rttr::registration::class_<FlowNodeProvider<MqttBrokerNode, "mqtt-broker", NodeKind::STANDALONE>>(
        "edgelink::plugins::modbus::MqttBrokerNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink::plugins::mqtt