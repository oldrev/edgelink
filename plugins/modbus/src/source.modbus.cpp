#include "pch.hpp"

#include <modbus/modbus.h>

#include <edgelink/plugin.hpp>

using namespace std;
using namespace edgelink;

namespace edgelink::plugins::modbus {

class ModbusLogNode : public SinkNode {
  public:
    ModbusLogNode(uint32_t id, const ::nlohmann::json& config, const INodeDescriptor* desc,
                  const std::vector<OutputPort>& output_ports, IMsgRouter* router)
        : edgelink::SinkNode(id, desc, output_ports, router) {}

    void start() override {}

    void stop() override {}

    void receive(const shared_ptr<Msg>& msg) override {
        //
        spdlog::info("LogNode > 收到了消息：[msg.id={0}, msg.birth_place=(id={1}, type='{2}')]，消息载荷：\n{3}",
                     msg->id, msg->birth_place->id(), msg->birth_place->descriptor()->type_name(),
                     msg->payload.dump(4));
    }
};

RTTR_PLUGIN_REGISTRATION {
    rttr::registration::class_<NodeProvider<ModbusLogNode, "modbus.log", NodeKind::SINK>>(
        "edgelink::plugins::modbus::ModbusLogNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink::plugins::modbus