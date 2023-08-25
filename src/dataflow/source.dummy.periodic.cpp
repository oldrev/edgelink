#include "../pch.h"

using namespace std;

namespace edgelink {

class DummyPeriodicSource : public virtual ISourceNode {
  public:
    DummyPeriodicSource(const ::nlohmann::json::object_t& config) {}

    void start() override {}

    void stop() override {}
};

struct DummyPeriodicSourceProvider : public virtual ISourceProvider {
  public:
    DummyPeriodicSourceProvider() : _type_name("source.dummy.periodic") { Engine::register_source(this); }

    const std::string& type_name() const override { return _type_name; }
    ISourceNode* create(const ::nlohmann::json::object_t& config) const override { return new DummyPeriodicSource(config); }

  private:
    const string _type_name;
};

const static DummyPeriodicSourceProvider s_dummy_periodic_source_provider;

}; // namespace edgelink