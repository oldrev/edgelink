#pragma once

#include <edgelink/plugin.hpp>

namespace edgelink::plugins::mqtt {

using Endpoint = async_mqtt::endpoint<async_mqtt::role::client, boost::asio::basic_stream_socket<boost::asio::ip::tcp>>;

/**
 * 同步的简单 MQTT 客户端接口实现
 */
class MqttClient : public std::enable_shared_from_this<MqttClient> {
  public:
    explicit MqttClient(const std::string_view host, uint16_t port);
    virtual ~MqttClient();

    Awaitable<void> async_connect();
    Awaitable<void> async_close() noexcept;

    bool is_connected() const { return _endpoint && _endpoint->next_layer().is_open(); }

    Awaitable<void> async_publish(const std::string_view topic, const async_mqtt::buffer& payload_buffer,
                                  async_mqtt::qos qos);
    Awaitable<void> async_publish_string(const std::string_view topic, const std::string_view payload,
                                         async_mqtt::qos qos);

    const std::string_view host() const noexcept { return _host; }
    uint16_t address() const noexcept { return _port; }

  private:
    std::string _host;
    uint16_t _port;
    std::unique_ptr<Endpoint> _endpoint;
};

}; // namespace edgelink::plugins::mqtt