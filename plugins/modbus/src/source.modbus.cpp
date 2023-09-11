#include <edgelink/plugin.hpp>

using namespace edgelink;

namespace edgelink::plugins::modbus {

class ModbusLogNode : public SinkNode {
  public:
    ModbusLogNode(const std::string_view id, const boost::json::object& config, const INodeDescriptor* desc,
                  const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : edgelink::SinkNode(id, desc, move(output_ports), flow, config) {}

    Awaitable<void> async_start() override { co_return; }

    Awaitable<void> async_stop() override { co_return; }

    Awaitable<void> receive_async(std::shared_ptr<Msg> msg) override {
        //
        spdlog::info("ModBusLogNode > 收到了消息：\n{0}", msg->to_string());
        co_return;
    }
};

RTTR_PLUGIN_REGISTRATION {
    rttr::registration::class_<FlowNodeProvider<ModbusLogNode, "modbus.log", NodeKind::SINK>>(
        "edgelink::plugins::modbus::ModbusLogNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink::plugins::modbus