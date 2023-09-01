#include "../../pch.hpp"
#include "edgelink/edgelink.hpp"

using namespace std;

namespace edgelink {

class LoggedSink : public SinkNode {
  public:
    LoggedSink(uint32_t id, const ::nlohmann::json& config, const INodeDescriptor* desc,
               const std::vector<OutputPort>& output_ports, IMsgRouter* router)
        : SinkNode(id, desc, output_ports, router) {}

    void start() override {}

    void stop() override {}

    void receive(const shared_ptr<Msg>& msg) override {
        //
        spdlog::info("LoggerSink > 收到了消息：[msg.id={0}]", msg->id);
    }
};

RTTR_REGISTRATION {
    rttr::registration::class_<NodeProvider<LoggedSink, "sink.logged", NodeKind::SINK>>(
        "edgelink::LoggedSinkProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink