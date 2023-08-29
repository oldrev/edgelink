#pragma once

namespace edgelink {

/// @brief 单向转发管道
class ForwardPipe : public virtual AbstractPipe {
  public:
    ForwardPipe(const ::nlohmann::json& config, IDataFlowNode* input, IDataFlowNode* output)
        : AbstractPipe(config, input, output), _condition(config["@condition"]) {
        //
    }

    // TODO FIXME 增加条件判断
    bool is_match(const Msg* data) const override { return true; }

    const std::string_view condition() const { return _condition; }

  private:
    const std::string _condition;
};

}; // namespace edgelink