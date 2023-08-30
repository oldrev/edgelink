#include "../pch.hpp"

#include <modbus/modbus.h>

#include "edgelink/edgelink.hpp"
#include "edgelink/transport/modbus.hpp"

using namespace std;

namespace edgelink {

class BaseModbusSource : public AbstractSource {
  public:
    BaseModbusSource(const ::nlohmann::json& config, const INodeDescriptor* desc, IMsgRouter* router)
        : AbstractSource(desc, router) {}
};

class ModbusRtuSource : public BaseModbusSource {
  public:
    ModbusRtuSource(const ::nlohmann::json& config, const INodeDescriptor* desc, IMsgRouter* router)
        : BaseModbusSource(config, desc, router) {}

  protected:
    void process(std::stop_token& stoken) override {}
};

class ModbusTcpSource : public BaseModbusSource {
  public:
    ModbusTcpSource(const ::nlohmann::json& config, const INodeDescriptor* desc, IMsgRouter* router)
        : BaseModbusSource(config, desc, router) {}

  protected:
    void process(std::stop_token& stoken) override {}
};

struct ModbusRtuSourceProvider : public INodeProvider, public INodeDescriptor {
    ModbusRtuSourceProvider() : _type_name("source.modbus.rtu") {}

    IDataFlowNode* create(const ::nlohmann::json& config, IMsgRouter* router) const override {
        return new ModbusRtuSource(config, this->descriptor(), router);
    }

    const INodeDescriptor* descriptor() const override { return this; }
    const std::string_view& type_name() const override { return _type_name; }
    const NodeKind kind() const override { return NodeKind::SOURCE; }

  private:
    const string_view _type_name;

    RTTR_ENABLE(INodeProvider)
};

}; // namespace edgelink

RTTR_REGISTRATION {
    rttr::registration::class_<edgelink::ModbusRtuSourceProvider>("edgelink::ModbusRtuSourceProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
}