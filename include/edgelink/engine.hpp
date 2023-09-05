#pragma once

namespace edgelink {

struct IFlowNode;

struct EngineConfig {};

using MsgRoutingPath = boost::container::static_vector<const IFlowNode*, 32>;

class Engine : public IEngine {
  public:
    explicit Engine(const boost::json::object& json_config, const IRegistry& registry);
    virtual ~Engine();

    const std::string_view id() const override { return _id; }
    const std::string_view name() const override { return _name; }

    Awaitable<void> start_async() override;
    Awaitable<void> stop_async() override;

    Awaitable<void> emit_async(FlowNodeID source_node_id, std::shared_ptr<Msg> msg) override;

    Awaitable<void> relay_async(FlowNodeID source_node_id, std::shared_ptr<Msg> msg, size_t port,
                                bool clone) const override;

    inline IFlowNode* get_node(FlowNodeID id) const override { return _nodes[static_cast<size_t>(id)].get(); }

  private:
    std::vector<std::unique_ptr<IFlowNode>> _nodes;
    const EngineConfig _config;

    std::unique_ptr<std::stop_source> _stop_source;
    const std::string _id;
    const std::string _name;
};

}; // namespace edgelink