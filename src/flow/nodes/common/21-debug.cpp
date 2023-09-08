#include "edgelink/edgelink.hpp"

namespace edgelink {

/*
    {
        "id": "adde85bf75a42c9c",
        "type": "debug",
        "z": "73e0fcd142fc5256",
        "name": "debug 1",
        "active": true,
        "tosidebar": true,
        "console": false,
        "tostatus": false,
        "complete": "true",
        "targetType": "full",
        "statusVal": "",
        "statusType": "auto",
        "x": 920,
        "y": 380,
        "wires": []
    }
*/

class DebugNode : public SinkNode {
  public:
    DebugNode(const std::string_view id, const boost::json::object& config, const INodeDescriptor* desc,
            const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : SinkNode(id, desc, std::move(output_ports), flow, config) {}

    Awaitable<void> start_async() override { co_return; }

    Awaitable<void> stop_async() override { co_return; }

    Awaitable<void> receive_async(std::shared_ptr<Msg> msg) override {
        //
        spdlog::info("DebugNode > {0}", msg->to_string());
        co_return;
    }
};

RTTR_REGISTRATION {
    rttr::registration::class_<FlowNodeProvider<DebugNode, "debug", NodeKind::SINK>>("edgelink::DebugNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink