#include "../pch.h"

using namespace std;

namespace edgelink {

class LoggedSink : public virtual ISinkNode {
  public:
    LoggedSink(const ::nlohmann::json::object_t& config) {}

    void start() override {}

    void stop() override {}
};

struct LoggedSinkProvider : public virtual ISinkProvider {
  public:
    LoggedSinkProvider() : _type_name("sink.logged") { Engine::register_sink(this); }

    const std::string& type_name() const override { return _type_name; }
    ISinkNode* create(const ::nlohmann::json::object_t& config) const override { return new LoggedSink(config); }

  private:
    const string _type_name;
};

const static LoggedSinkProvider s_logged_sink_provider;

}; // namespace edgelink