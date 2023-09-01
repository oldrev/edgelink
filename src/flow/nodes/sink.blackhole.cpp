#include "../../pch.hpp"
#include "edgelink/edgelink.hpp"

using namespace std;

namespace edgelink {

class BlackholeSink : public SinkNode {
  public:
    BlackholeSink(uint32_t id, const ::nlohmann::json& config, const INodeDescriptor* desc,
                  const std::vector<OutputPort>& output_ports, IMsgRouter* router)
        : SinkNode(id, desc, output_ports, router) {}

    void start() override {}

    void stop() override {}

    void receive(const std::shared_ptr<Msg>& msg) override {
        spdlog::info("BlackholeSink > 吃掉了消息：[msg.id={0}]", msg->id);
    }
};

RTTR_REGISTRATION {
    rttr::registration::class_<NodeProvider<BlackholeSink, "sink.blackhole", NodeKind::SINK>>(
        "edgelink::BlackholeSinkProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink
