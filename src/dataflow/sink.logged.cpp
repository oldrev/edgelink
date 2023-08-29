#include "../pch.hpp"
#include "edgelink/edgelink.hpp"

using namespace std;

namespace edgelink {

class LoggedSink : public AbstractSink {
  public:
    LoggedSink(const ::nlohmann::json& config, IMsgRouter* router) : AbstractSink(router) {}

    void start() override {}

    void stop() override {}

    void receive(const Msg* msg) override {
        decimal64 count = std::get<decimal64>(msg->payload.at("count"));
        spdlog::info("LoggerSink: 收到了消息：[counter={0}]", count);
    }
};

class LoggedSinkProvider : public INodeProvider {
  public:
    LoggedSinkProvider() : _type_name("sink.logged") {}

    const std::string_view& type_name() const override { return _type_name; }

    IDataFlowNode* create(const ::nlohmann::json& config, IMsgRouter* router) const override {
        return new LoggedSink(config, router);
    }

  private:
    const string_view _type_name;

    RTTR_ENABLE(INodeProvider)
};

}; // namespace edgelink

RTTR_REGISTRATION {
    rttr::registration::class_<edgelink::LoggedSinkProvider>("edgelink::LoggedSinkProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
}