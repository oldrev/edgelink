#include "../pch.hpp"

#include <modbus/modbus.h>

#include "edgelink/edgelink.hpp"
#include "edgelink/transport/modbus.hpp"

using namespace std;

namespace edgelink {

class BaseModbusSource : public ISourceNode {

    void start() override {}

    void stop() override {}
};

class ModbusRtuSource : public BaseModbusSource {
  public:
    ModbusRtuSource(const ::nlohmann::json& config) {}
};

class ModbusTcpSource : public virtual BaseModbusSource {
  public:
    ModbusTcpSource(const ::nlohmann::json::object_t& config) {}
};

struct ModbusRtuSourceProvider : public ISourceProvider {
    ModbusRtuSourceProvider() : _type_name("source.modbus.rtu") {}

    const std::string_view& type_name() const override { return _type_name; }
    ISourceNode* create(const ::nlohmann::json& config) const override { return new ModbusRtuSource(config); }

  private:
    const string_view _type_name;

    RTTR_ENABLE(ISourceProvider)
};

}; // namespace edgelink

RTTR_REGISTRATION {
    rttr::registration::class_<edgelink::ModbusRtuSourceProvider>("edgelink::ModbusRtuSourceProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
}