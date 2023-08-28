#include "../pch.hpp"
#include "edgelink/edgelink.hpp"

using namespace std;

namespace edgelink {

class LoggedSink : public ISinkNode {
  public:
    LoggedSink(const ::nlohmann::json& config) {}

    void start() override {}

    void stop() override {}

    void receive(const Msg* msg) override {
        decimal64 count = std::get<decimal64>(msg->payload.at("count"));
        spdlog::info("LoggerSink: 收到了消息：[counter={0}]", count);
    }
};

class LoggedSinkProvider : public ISinkProvider {
  public:
    LoggedSinkProvider() : _type_name("sink.logged") {}

    const std::string_view& type_name() const override { return _type_name; }
    ISinkNode* create(const ::nlohmann::json& config) const override { return new LoggedSink(config); }

  private:
    const string_view _type_name;

    RTTR_ENABLE(ISinkProvider)
};


}; // namespace edgelink

RTTR_REGISTRATION {
    rttr::registration::class_<edgelink::LoggedSinkProvider>("edgelink::LoggedSinkProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
}