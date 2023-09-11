#include "edgelink/edgelink.hpp"

namespace edgelink {

class CompleteNode : public FlowNode {
  public:
    CompleteNode(const std::string_view id, const boost::json::object& config, const INodeDescriptor* desc,
                 const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : FlowNode(id, desc, std::move(output_ports), flow, config),
          _scope(std::move(CompleteNode::config_scope(config))) {
        // TODO 挂接 scope 里节点的连接事件
    }

    virtual ~CompleteNode() {
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
        for (auto&& jv : config.at("scope").as_array()) {
            scope.emplace_back(jv.as_string());
        }
        return scope;
    }

  private:
    const std::vector<std::string> _scope;
};

RTTR_REGISTRATION {
    rttr::registration::class_<FlowNodeProvider<CompleteNode, "complete", NodeKind::SOURCE>>(
        "edgelink::CompleteNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink