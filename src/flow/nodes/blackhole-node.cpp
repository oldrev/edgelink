#include "../../pch.hpp"
#include "edgelink/edgelink.hpp"

using namespace std;

namespace edgelink {

class BlackholeNode : public SinkNode {
  public:
    BlackholeNode(uint32_t id, const ::nlohmann::json& config, const INodeDescriptor* desc,
                  const std::vector<OutputPort>& output_ports, IMsgRouter* router)
        : SinkNode(id, desc, output_ports, router) {}

    void start() override {}

    void stop() override {}

    void receive(const std::shared_ptr<Msg>& msg) override {
        spdlog::info("BlackholeNode > 吃掉了消息：[msg.id={0}]", msg->id);
    }
};

RTTR_REGISTRATION {
    rttr::registration::class_<NodeProvider<BlackholeNode, "blackhole", NodeKind::SINK>>(
        "edgelink::BlackholeNodeProvider")
        .constructor()(rttr::policy::ctor::as_std_shared_ptr);
};

}; // namespace edgelink
