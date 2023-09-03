#pragma once

#include <edgelink/plugin.hpp>

namespace edgelink::plugins::mqtt {

/**
 * 同步的简单 MQTT 客户端接口实现
 */
class MqttClient : public ::mqtt::callback, public edgelink::IClosable {
  public:
    MqttClient(const ::nlohmann::json& json_config);
    virtual ~MqttClient();
    void connect();
    void close() noexcept override;

    void publish(const std::string_view& topic, ::mqtt::binary_ref payload, int qos);

    const std::string_view address() const noexcept { return _address; }
    bool is_connected() const noexcept { return _mqtt->is_connected(); }

  private:
    std::string _address;
    std::unique_ptr<::mqtt::client> _mqtt;
};

}; // namespace edgelink::plugins::mqtt