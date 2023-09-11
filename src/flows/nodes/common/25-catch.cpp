#include "edgelink/edgelink.hpp"

namespace edgelink {

class CatchNode : public FlowNode {
  public:
    CatchNode(const std::string_view id, const boost::json::object& config, const INodeDescriptor* desc,
              const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : FlowNode(id, desc, std::move(output_ports), flow, config),
          _scope(std::move(CatchNode::config_scope(config))), _uncaught(config.at("uncaught").as_bool()) {
        // TODO 挂接 scope 里节点的连接事件
    }

    virtual ~CatchNode() {
        // TODO 断开连接
    }

    Awaitable<void> async_start() override { co_return; }

    Awaitable<void> async_stop() override { co_return; }

    Awaitable<void> receive_async(std::shared_ptr<Msg> msg) override {
        throw NotSupportedException("'complete' 节点不支持作为目的节点");
        co_return;
    }

    const std::vector<std::string>& scope() { return _scope; }

  private:
    static std::vector<std::string> config_scope(const boost::json::object& config) {
        std::vector<std::string> scope;
        auto& scope_value = config.at("scope");
        if (scope_value.is_array()) {
            for (auto&& jv : scope_value.as_array()) {
                scope.emplace_back(jv.as_string());
            }
        }
        return scope;
    }

  private:
  const bool _uncaught;
    const std::vector<std::string> _scope;
};

RTTR_REGISTRATION {
    rttr::registration::class_<FlowNodeProvider<CatchNode, "catch", NodeKind::SOURCE>>(
        "edgelink::CatchNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink