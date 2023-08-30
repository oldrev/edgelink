#pragma once

namespace edgelink {

struct EngineConfig {
    size_t queue_capacity;
};

using MsgRoutingPath = boost::container::static_vector<const IDataFlowNode*, 32>;

class Engine : public IEngine {
  public:
    explicit Engine(const ::nlohmann::json& json_config);
    virtual ~Engine();

    void run() override;

    void emit(std::shared_ptr<Msg> msg) override;

    inline uint64_t generate_msg_id() override { return _msg_id_counter.fetch_add(1); }

  private:
    void do_dfs(const IDataFlowNode* current, MsgRoutingPath& path, const std::shared_ptr<Msg>& orig_msg);
    void worker_proc(std::stop_token stoken);

  private:
    std::vector<IDataFlowNode*> _nodes;
    std::vector<Pipe*> _pipes;
    const EngineConfig _config;
    boost::sync_bounded_queue<std::shared_ptr<Msg>> _msg_queue;

    std::map<std::string_view, const INodeProvider*> _node_providers;

    std::atomic<uint64_t> _msg_id_counter; // 初始化计数器为0

};

}; // namespace edgelink