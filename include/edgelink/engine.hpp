#pragma once

namespace edgelink {

struct IFlowNode;

struct EngineConfig {
};

using MsgRoutingPath = boost::container::static_vector<const IFlowNode*, 32>;

class Engine : public IEngine {
  public:
    explicit Engine(const ::nlohmann::json& json_config);
    virtual ~Engine();

    void run() override;
    void start() override;
    void stop() override;

    void emit(std::shared_ptr<Msg> msg) override;

    void relay(const FlowNode* source, std::shared_ptr<Msg> msg, bool clone = true) const override;

    inline uint64_t generate_msg_id() override { return _msg_id_counter.fetch_add(1); }

    const std::vector<const Wire*>& node_wires(const FlowNode* node) const override {
        return _node_wires.at(node);
    }

  private:
    std::vector<FlowNode*> _nodes;
    std::vector<Wire*> _wires;
    const EngineConfig _config;
    std::map<const FlowNode*, std::vector<const Wire*>> _node_wires;

    std::map<std::string_view, const INodeProvider*> _node_providers;

    std::atomic<uint64_t> _msg_id_counter; // 初始化计数器为0

    std::unique_ptr<std::stop_source> _stop_source;
    std::unique_ptr<boost::asio::thread_pool> _pool;
};

}; // namespace edgelink