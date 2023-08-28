#pragma once

namespace edgelink {

/// @brief 单向转发管道
class ForwardPipe : public virtual AbstractPipe {
  public:
    ForwardPipe(const ::nlohmann::json& config, IDataFlowNode* from, IDataFlowNode* to)
        : AbstractPipe(config, from, to) {}

    // TODO FIXME 增加条件判断
    bool is_match(const Msg* data) const override { return true; }
};

}; // namespace edgelink