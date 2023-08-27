#include "../pch.hpp"

#include "edgelink/edgelink.hpp"

using namespace std;

namespace edgelink {

/// @brief 单向转发管道
class ForwardPipe : public virtual AbstractPipe {
  public:
    ForwardPipe(const ::nlohmann::json::object_t& config, IDataFlowElement* from, IDataFlowElement* to)
        : AbstractPipe(config, from, to) {}

    // TODO FIXME
    bool is_match(const Msg* data) const override { return true; }
};

/// @brief 单向转发管道提供者
struct ForwardPipeProvider : public virtual IPipeProvider {
  public:
    ForwardPipeProvider() : _type_name("filter.forward") { Engine::register_filter(this); }

    const std::string& type_name() const override { return _type_name; }

    IPipe* create(const ::nlohmann::json::object_t& config, IDataFlowElement* from,
                  IDataFlowElement* to) const override {
        return new ForwardPipe(config, from, to);
    }

  private:
    const string _type_name;
};

const static ForwardPipeProvider s_fpp;

}; // namespace edgelink