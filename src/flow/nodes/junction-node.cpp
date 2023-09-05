#include "edgelink/edgelink.hpp"

namespace edgelink {

class JunctionNode : public FlowNode {
  public:
    JunctionNode(FlowNodeID id, const boost::json::object& config, const INodeDescriptor* desc,
                 const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : FlowNode(id, desc, std::move(output_ports), flow, config) {}

    Awaitable<void> start_async() override { co_return; }

    Awaitable<void> stop_async() override { co_return; }

    Awaitable<void> receive_async(std::shared_ptr<Msg> msg) override {
        // 直接分发消息
        co_await this->flow()->relay_async(this->id(), msg, 0, true);
    }
};

RTTR_REGISTRATION {
    rttr::registration::class_<NodeProvider<JunctionNode, "junction", NodeKind::JUNCTION>>(
        "edgelink::JunctionNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink