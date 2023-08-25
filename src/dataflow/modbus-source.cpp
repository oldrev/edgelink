#include "../pch.h"

#include "edgelink/transport/modbus.hpp"

namespace edgelink {

class BaseModbusSource : public virtual ISourceNode {

    void start() override {}

    void stop() override {}
};

class ModbusRtuSource : public virtual BaseModbusSource {
  public:
    ModbusRtuSource(const ::nlohmann::json::object_t& config) {}
};

class ModbusTcpSource : public virtual BaseModbusSource {
  public:
    ModbusTcpSource(const ::nlohmann::json::object_t& config) {}
};

static struct DescriptorRegister {
  public:
    DescriptorRegister() {
        Engine::register_source("source.modbus.rtu", [](const auto& config) { return new ModbusRtuSource(config); });
        Engine::register_source("source.modbus.tcp", [](const auto& config) { return new ModbusTcpSource(config); });
    }
} s_desc;

}; // namespace edgelink