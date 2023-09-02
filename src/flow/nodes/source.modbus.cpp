#include "../../pch.hpp"

#include <modbus/modbus.h>

#include "edgelink/edgelink.hpp"
#include "edgelink/transport/modbus.hpp"

using namespace std;

namespace edgelink {

/*
class BaseModbusSource : public SourceNode {
  protected:
    BaseModbusSource(const ::nlohmann::json& config, const INodeDescriptor* desc, IMsgRouter* router)
        : SourceNode(desc, router) {}

    void process(std::stop_token& stoken) = 0;
};

class ModbusRtuSource final : public BaseModbusSource {
  public:
    ModbusRtuSource(const ::nlohmann::json& config, const INodeDescriptor* desc, IMsgRouter* router)
        : BaseModbusSource(config, desc, router) {}

  protected:
    void process(std::stop_token& stoken) override {}
};

class ModbusTcpSource final : public BaseModbusSource {
  public:
    ModbusTcpSource(const ::nlohmann::json& config, const INodeDescriptor* desc, IMsgRouter* router)
        : BaseModbusSource(config, desc, router) {}

  protected:
    void process(std::stop_token& stoken) override {}
};

RTTR_REGISTRATION {
    rttr::registration::class_<NodeProvider<ModbusRtuSource, "source.modbus.rtu", NodeKind::SOURCE>>(
        "edgelink::ModbusRtuSourceProvider")
        .constructor()(rttr::policy::ctor::as_std_shared_ptr);

    rttr::registration::class_<NodeProvider<ModbusTcpSource, "source.modbus.tcp", NodeKind::SOURCE>>(
        "edgelink::ModbusTcpSourceProvider")
        .constructor()(rttr::policy::ctor::as_std_shared_ptr);
};
*/

}; // namespace edgelink
