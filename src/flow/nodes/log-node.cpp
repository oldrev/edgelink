#include "../../pch.hpp"
#include "edgelink/edgelink.hpp"

using namespace std;

namespace edgelink {

class LogNode : public SinkNode {
  public:
    LogNode(uint32_t id, const ::nlohmann::json& config, const INodeDescriptor* desc,
            const std::vector<OutputPort>&& output_ports, IFlow* router)
        : SinkNode(id, desc, move(output_ports), router) {}

    void start() override {}

    void stop() override {}

    void receive(const shared_ptr<Msg>& msg) override {
        //
        spdlog::info("LogNode > 收到了消息：[msg.id={0}, msg.birth_place=(id={1}, type='{2}')]，消息载荷：\n{3}",
                     msg->id, msg->birth_place->id(), msg->birth_place->descriptor()->type_name(),
                     msg->payload.dump(4));
    }
};

RTTR_REGISTRATION {
    rttr::registration::class_<NodeProvider<LogNode, "log", NodeKind::SINK>>("edgelink::LogNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink