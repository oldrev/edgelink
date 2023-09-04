#include "edgelink/edgelink.hpp"

namespace edgelink {

class LogNode : public SinkNode {
  public:
    LogNode(FlowNodeID id, const ::nlohmann::json& config, const INodeDescriptor* desc,
            const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : SinkNode(id, desc, std::move(output_ports), flow) {}

    Awaitable<void> start_async() override { co_return; }

    Awaitable<void> stop_async() override { co_return; }

    Awaitable<void> receive_async(std::shared_ptr<Msg> msg) override {
        //
        FlowNodeID node_id = msg->data().at("birthPlaceID");
        auto birth_place = this->flow()->get_node(node_id);
        spdlog::info("LogNode > 收到了消息：\n{0}", msg->data().dump(4));
        co_return;
    }
};

RTTR_REGISTRATION {
    rttr::registration::class_<NodeProvider<LogNode, "log", NodeKind::SINK>>("edgelink::LogNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink