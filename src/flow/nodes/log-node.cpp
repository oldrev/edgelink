#include "edgelink/edgelink.hpp"

namespace edgelink {

class LogNode : public SinkNode {
  public:
    LogNode(const std::string_view id, const boost::json::object& config, const INodeDescriptor* desc,
            const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : SinkNode(id, desc, std::move(output_ports), flow, config) {}

    Awaitable<void> start_async() override { co_return; }

    Awaitable<void> stop_async() override { co_return; }

    Awaitable<void> receive_async(std::shared_ptr<Msg> msg) override {
        //
        const std::string_view node_id = msg->data().at("birthPlaceID").as_string();
        auto birth_place = this->flow()->get_node(node_id);
        spdlog::info("LogNode > 收到了消息：{0}", msg->to_string());
        co_return;
    }
};

RTTR_REGISTRATION {
    rttr::registration::class_<FlowNodeProvider<LogNode, "log", NodeKind::SINK>>("edgelink::LogNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink