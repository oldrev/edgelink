#include "../pch.hpp"
#include "edgelink/edgelink.hpp"

using namespace std;

namespace edgelink {

class BlackholeSink : public AbstractSink {
  public:
    BlackholeSink(const ::nlohmann::json& config, const INodeDescriptor* desc, IMsgRouter* router)
        : AbstractSink(desc, router) {}

    void start() override {}

    void stop() override {}

    void receive(const Msg* msg) override {}
};

RTTR_REGISTRATION {
    rttr::registration::class_<NodeProvider<BlackholeSink, "sink.blackhole", NodeKind::SINK>>(
        "edgelink::BlackholeSinkProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink
