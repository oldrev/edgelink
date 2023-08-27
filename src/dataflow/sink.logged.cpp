#include "../pch.hpp"
#include "edgelink/edgelink.hpp"

using namespace std;

namespace edgelink {

class LoggedSink : public virtual ISinkNode {
  public:
    LoggedSink(const ::nlohmann::json& config) {}

    void start() override {}

    void stop() override {}
};

class LoggedSinkProvider : public virtual ISinkProvider {
  public:
    LoggedSinkProvider() : _type_name("sink.logged") {}

    const std::string_view& type_name() const override { return _type_name; }
    ISinkNode* create(const ::nlohmann::json& config) const override { return new LoggedSink(config); }

  private:
    const string_view _type_name;

    RTTR_ENABLE(ISinkProvider)
};


}; // namespace edgelink

RTTR_REGISTRATION { rttr::registration::class_<edgelink::LoggedSinkProvider>("edgelink::LoggedSinkProvider"); }