#pragma once

#include "edgelink/utils.hpp"

namespace edgelink {

class FlowNode;
struct IRegistry;
}; // namespace edgelink

namespace edgelink::flow::details {

class Flow : public IFlow {

  public:
    Flow(const std::string& id);
    virtual ~Flow();

    const std::string& id() const override { return _id; }

    Awaitable<void> start_async() override;
    Awaitable<void> stop_async() override;

    Awaitable<void> emit_async(FlowNodeID source_node_id, std::shared_ptr<Msg> msg) override;

    Awaitable<void> relay_async(FlowNodeID source_node_id, std::shared_ptr<Msg> msg, size_t port,
                                bool clone) const override;

    inline IFlowNode* get_node(FlowNodeID id) const override { return _nodes[static_cast<size_t>(id)].get(); }

    inline void emplace_node(std::unique_ptr<IFlowNode>&& node) { _nodes.emplace_back(std::move(node)); }

  private:
    const std::string _id;
    std::vector<std::unique_ptr<IFlowNode>> _nodes;

    std::atomic<uint64_t> _msg_id_counter; // 初始化计数器为0

    std::unique_ptr<std::stop_source> _stop_source;
};

}; // namespace edgelink::flow::details