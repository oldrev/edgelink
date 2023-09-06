#pragma once

#include <edgelink/plugin.hpp>

namespace edgelink::plugins::mqtt {

using Endpoint = async_mqtt::endpoint<async_mqtt::role::client, boost::asio::basic_stream_socket<boost::asio::ip::tcp>>;

/**
 * 同步的简单 MQTT 客户端接口实现
 */
class MqttClient : public std::enable_shared_from_this<MqttClient> {
  public:
    explicit MqttClient(const boost::url& address);
    virtual ~MqttClient();

    Awaitable<void> async_connect();
    Awaitable<void> async_close() noexcept;

    bool is_connected() const { return true; }

    Awaitable<void> publish_async(const std::string_view topic, const std::string_view payload, async_mqtt::qos qos);

    const boost::url address() const noexcept { return _address; }

  private:
    boost::url _address;
    std::unique_ptr<Endpoint> _endpoint;
};

}; // namespace edgelink::plugins::mqtt