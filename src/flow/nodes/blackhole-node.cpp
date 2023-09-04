#include "edgelink/edgelink.hpp"

namespace edgelink {

class BlackholeNode : public SinkNode {
  public:
    BlackholeNode(FlowNodeID id, const boost::json::object& config, const INodeDescriptor* desc,
                  const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : SinkNode(id, desc, std::move(output_ports), flow) {}

    Awaitable<void> start_async() override { co_return; }

    Awaitable<void> stop_async() override { co_return; }

    Awaitable<void> receive_async(std::shared_ptr<Msg> msg) override {
        //
        FlowNodeID msg_id = msg->data().at("id").to_number<FlowNodeID>();
        spdlog::info("BlackholeNode > 吃掉了消息：[msg.id={0}]", msg_id);
        co_return;
    }
};

RTTR_REGISTRATION {
    rttr::registration::class_<NodeProvider<BlackholeNode, "blackhole", NodeKind::SINK>>(
        "edgelink::BlackholeNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink
