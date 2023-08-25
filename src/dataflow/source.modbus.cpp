#include "../pch.h"

#include "edgelink/transport/modbus.hpp"

using namespace std;

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

struct ModbusRtuSourceProvider : public virtual ISourceProvider {
    ModbusRtuSourceProvider() : _type_name("source.modbus.rtu") { Engine::register_source(this); }

    const std::string& type_name() const override { return _type_name; }
    ISourceNode* create(const ::nlohmann::json::object_t& config) const override { return new ModbusRtuSource(config); }

  private:
    const string _type_name;

};

const static ModbusRtuSourceProvider s_rtu_source_provider;

}; // namespace edgelink