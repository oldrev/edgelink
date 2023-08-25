#include "../pch.h"

using namespace std;

namespace edgelink {

class ForwardFilter : public virtual AbstractFilter {
  public:
    ForwardFilter(const ::nlohmann::json::object_t& config, ISourceNode* source, ISinkNode* sink)
        : AbstractFilter(config, source, sink) {}

    void start() override {}

    void stop() override {}

    bool is_match(const std::span<uint8_t>& data) const override { return true; }
};

struct ForwardFilterProvider : public virtual IFilterProvider {
  public:
    ForwardFilterProvider() : _type_name("filter.forward") { Engine::register_filter(this); }

    const std::string& type_name() const override { return _type_name; }
    IFilter* create(const ::nlohmann::json::object_t& config, ISourceNode* source, ISinkNode* sink) const {
        return new ForwardFilter(config, source, sink);
    }

  private:
    const string _type_name;
};

const static ForwardFilterProvider s_ffp;

}; // namespace edgelink