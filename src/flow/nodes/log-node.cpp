#include "edgelink/edgelink.hpp"

using namespace std;

namespace edgelink {

class LogNode : public SinkNode {
  public:
    LogNode(uint32_t id, const ::nlohmann::json& config, const INodeDescriptor* desc,
            const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : SinkNode(id, desc, move(output_ports), flow) {}

    void start() override {}

    void stop() override {}

    void receive(shared_ptr<Msg> msg) override {
        //
        uint32_t node_id = msg->data().at("birthPlaceID");
        auto birth_place = this->flow()->get_node(node_id);
        spdlog::info("LogNode > 收到了消息：\n{0}", msg->data().dump(4));
    }
};

RTTR_REGISTRATION {
    rttr::registration::class_<NodeProvider<LogNode, "log", NodeKind::SINK>>("edgelink::LogNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink