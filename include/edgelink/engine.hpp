#pragma once

namespace edgelink {

struct EngineConfig {
    size_t queue_capacity;
};

using MsgRoutingPath = boost::container::static_vector<IDataFlowNode*, 32>;

class Engine : public IEngine {
  public:
    explicit Engine(const ::nlohmann::json& json_config);
    virtual ~Engine();

    void run() override;
    void emit(Msg* msg) override;

  private:
    void do_dfs(IDataFlowNode* current, MsgRoutingPath& path, Msg* msg);
    void worker_proc(std::stop_token stoken);

  private:
    std::vector<IDataFlowNode*> _nodes;
    std::vector<IPipe*> _pipes;
    const EngineConfig _config;
    boost::sync_bounded_queue<Msg*> _msg_queue;

    std::map<std::string_view, const INodeProvider*> _node_providers;
};

}; // namespace edgelink