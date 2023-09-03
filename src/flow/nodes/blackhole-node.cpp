#include "edgelink/edgelink.hpp"

using namespace std;

namespace edgelink {

class BlackholeNode : public SinkNode {
  public:
    BlackholeNode(uint32_t id, const ::nlohmann::json& config, const INodeDescriptor* desc,
                  const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : SinkNode(id, desc, move(output_ports), flow) {}

    Awaitable<void> start_async() override { co_return; }

    Awaitable<void> stop_async() override { co_return; }

    Awaitable<void> receive_async(shared_ptr<Msg> msg) override {
        //
        uint32_t msg_id = msg->data().at("id");
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
