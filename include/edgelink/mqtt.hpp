#pragma once

#include "edgelink.hpp"

namespace edgelink {

/**
 * 同步的简单 MQTT 客户端接口实现
 */
class MqttClient : public virtual mqtt::callback, virtual public IClosable {
  public:
    MqttClient(const EdgeLinkSettings& settings);
    virtual ~MqttClient();
    Result<> connect() noexcept;
    void close() noexcept override;

    Result<> publish(const std::string_view& topic, mqtt::binary_ref payload, int qos) noexcept;

    const std::string_view address() const noexcept { return _address; }
    bool is_connected() const noexcept { return _mqtt->is_connected(); }

  private:
    std::string _address;
    std::unique_ptr<mqtt::client> _mqtt;
};

}; // namespace edgelink