#pragma once

#include <edgelink/plugin.hpp>

namespace edgelink::plugins::mqtt {

/**
 * 同步的简单 MQTT 客户端接口实现
 */
class MqttClient : public edgelink::IClosable {
  public:
    MqttClient(const boost::json::object& json_config);
    virtual ~MqttClient();
    void connect();
    void close() noexcept override;

    bool is_connected() const { return true; }

    void publish(const std::string_view& topic, const void* buf, int qos);

    const std::string_view address() const noexcept { return _address; }

  private:
    std::string _address;
};

}; // namespace edgelink::plugins::mqtt