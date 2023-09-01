#include "../../pch.hpp"
#include "edgelink/edgelink.hpp"

using namespace std;

namespace edgelink {

class LoggedSink : public SinkNode {
  public:
    LoggedSink(const ::nlohmann::json& config, const INodeDescriptor* desc, IMsgRouter* router)
        : SinkNode(desc, router) {}

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