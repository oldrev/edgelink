#include "../pch.hpp"

#include "edgelink/edgelink.hpp"

using namespace std;

namespace edgelink {

class DummyPeriodicSource : public virtual ISourceNode {
  public:
    DummyPeriodicSource(const ::nlohmann::json& config) {}

    void start() override {}

    void stop() override {}
};

struct DummyPeriodicSourceProvider : public virtual ISourceProvider {
  public:
    DummyPeriodicSourceProvider() : _type_name("source.dummy.periodic") {}

    const std::string_view& type_name() const override { return _type_name; }
    ISourceNode* create(const ::nlohmann::json& config) const override { return new DummyPeriodicSource(config); }

  private:
    const string_view _type_name;

    RTTR_ENABLE(ISourceProvider)
};

}; // namespace edgelink


RTTR_REGISTRATION { rttr::registration::class_<edgelink::DummyPeriodicSourceProvider>("edgelink::DummyPeriodicSourceProvider"); }