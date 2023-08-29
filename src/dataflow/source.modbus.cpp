#include "../pch.hpp"

#include <modbus/modbus.h>

#include "edgelink/edgelink.hpp"
#include "edgelink/transport/modbus.hpp"

using namespace std;

namespace edgelink {

class BaseModbusSource : public AbstractSource {
  public:
    BaseModbusSource(const ::nlohmann::json& config, IMsgRouter* router) : AbstractSource(router) {}
};

class ModbusRtuSource : public BaseModbusSource {
  public:
    ModbusRtuSource(const ::nlohmann::json& config, IMsgRouter* router) : BaseModbusSource(config, router) {}

  protected:
    void process(std::stop_token& stoken) override {}
};

class ModbusTcpSource : public virtual BaseModbusSource {
  public:
    ModbusTcpSource(const ::nlohmann::json& config, IMsgRouter* router) : BaseModbusSource(config, router) {}

  protected:
    void process(std::stop_token& stoken) override {}
};

struct ModbusRtuSourceProvider : public INodeProvider {
    ModbusRtuSourceProvider() : _type_name("source.modbus.rtu") {}

    const std::string_view& type_name() const override { return _type_name; }
    IDataFlowNode* create(const ::nlohmann::json& config, IMsgRouter* router) const override {
        return new ModbusRtuSource(config, router);
    }

  private:
    const string_view _type_name;

    RTTR_ENABLE(INodeProvider)
};

}; // namespace edgelink

RTTR_REGISTRATION {
    rttr::registration::class_<edgelink::ModbusRtuSourceProvider>("edgelink::ModbusRtuSourceProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
}