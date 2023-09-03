#include "edgelink/edgelink.hpp"

using namespace std;

namespace edgelink {

class JunctionNode : public FlowNode {
  public:
    JunctionNode(uint32_t id, const ::nlohmann::json& config, const INodeDescriptor* desc,
                 const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : FlowNode(id, desc, move(output_ports), flow) {}

    void start() override {}

    void stop() override {}

    void receive(shared_ptr<Msg> msg) override {
        // 直接分发消息
        this->flow()->relay(this->id(), msg, 0, true);
    }
};

RTTR_REGISTRATION {
    rttr::registration::class_<NodeProvider<JunctionNode, "junction", NodeKind::JUNCTION>>(
        "edgelink::JunctionNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink