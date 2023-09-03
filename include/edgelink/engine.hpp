#pragma once

namespace edgelink {

struct IFlowNode;

struct EngineConfig {};

using MsgRoutingPath = boost::container::static_vector<const IFlowNode*, 32>;

class Engine : public IEngine {
  public:
    explicit Engine(const ::nlohmann::json& json_config, const IRegistry& registry);
    virtual ~Engine();

    Awaitable<void> run_async() override;
    Awaitable<void> start_async() override;
    Awaitable<void> stop_async() override;

    Awaitable<void> emit_async(uint32_t source_node_id, std::shared_ptr<Msg> msg) override;

    Awaitable<void> relay_async(uint32_t source_node_id, std::shared_ptr<Msg> msg, size_t port,
                                bool clone) const override;

    inline uint64_t generate_msg_id() override { return _msg_id_counter.fetch_add(1); }
    inline FlowNode* get_node(uint32_t id) const override { return _nodes[static_cast<size_t>(id)].get(); }

  private:
    std::vector<std::unique_ptr<FlowNode>> _nodes;
    const EngineConfig _config;

    std::atomic<uint64_t> _msg_id_counter; // 初始化计数器为0

    std::unique_ptr<std::stop_source> _stop_source;
};

}; // namespace edgelink