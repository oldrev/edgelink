#include "../pch.hpp"

#include "edgelink/edgelink.hpp"

using namespace std;

namespace edgelink {

/// @brief 单向转发管道
class ForwardPipe : public virtual AbstractPipe {
  public:
    ForwardPipe(const ::nlohmann::json& config, IDataFlowElement* from, IDataFlowElement* to)
        : AbstractPipe(config, from, to) {}

    // TODO FIXME
    bool is_match(const Msg* data) const override { return true; }
};

}; // namespace edgelink