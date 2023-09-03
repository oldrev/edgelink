#include "pch.hpp"

#include <modbus/modbus.h>

#include <edgelink/plugin.hpp>

using namespace std;
using namespace edgelink;

namespace edgelink::plugins::modbus {

class ModbusLogNode : public SinkNode {
  public:
    ModbusLogNode(uint32_t id, const ::nlohmann::json& config, const INodeDescriptor* desc,
                  const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : edgelink::SinkNode(id, desc, move(output_ports), flow) {}

    void start() override {}

    void stop() override {}

    void receive(shared_ptr<Msg> msg) override {
        //
        spdlog::info("ModBusLogNode > 收到了消息：\n{0}", msg->data().at("payload"));
    }
};

RTTR_PLUGIN_REGISTRATION {
    rttr::registration::class_<NodeProvider<ModbusLogNode, "modbus.log", NodeKind::SINK>>(
        "edgelink::plugins::modbus::ModbusLogNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink::plugins::modbus