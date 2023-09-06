
#include "mqtt.client.hpp"

using namespace boost;
namespace asio = boost::asio;
namespace am = async_mqtt;
namespace this_coro = boost::asio::this_coro;

namespace edgelink::plugins::mqtt {

MqttClient::MqttClient(const boost::url& address) : _address(address) {
    //
    auto client_id = uuids::to_string(uuids::uuid());
}

MqttClient::~MqttClient() {}

Awaitable<void> MqttClient::async_connect() {
    spdlog::info("开始连接 MQTT：{0}:{1}", _address.host(), _address.port());

    auto exe = co_await this_coro::executor;

    {
        auto amep = new Endpoint{am::protocol_version::v3_1_1, exe};
        _endpoint = std::move(std::unique_ptr<Endpoint>(amep));
    }

    // asio::ip::tcp::socket resolve_sock{exe};
    asio::ip::tcp::resolver resolver(exe);

    // Resolve hostname
    spdlog::info("MqttClient > 解析地址");

    auto eps = co_await resolver.async_resolve(_address.host(), _address.port(), asio::use_awaitable);

    // Layer
    // am::stream -> TCP

    // Underlying TCP connect
    spdlog::info("MqttClient > socket 开始连接");
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
                                   // std::cout << "MQTT CONNACK recv" << " sp:" << p.session_present() << std::endl;
                                   // spdlog::info("MqttClient > 收到连接相应");
                               },
                               [](auto const&) {}};
        pv.visit(cb);
    } else {
        spdlog::error("MqttClient > CONNACK 收到错误：{0}", pv.get<am::system_error>().what());
        co_return;
    }

    spdlog::info("MQTT 已连接：{0}:{1}", _address.host(), _address.port());
}

Awaitable<void> MqttClient::async_close() noexcept {
    co_await _endpoint->close(asio::use_awaitable);
    spdlog::info("MQTT 连接已断开，主机：{0}:{1}", _address.host(), _address.port());
}

Awaitable<void> MqttClient::publish_async(const std::string_view topic, const std::string_view payload,
                                          async_mqtt::qos qos) {
    auto topic_buffer = am::allocate_buffer(topic);
    auto payload_buffer = am::allocate_buffer(payload);
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

}; // namespace edgelink::plugins::mqtt