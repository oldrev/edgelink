#pragma once

#include "dataflow.hpp"

namespace edgelink {

struct EngineConfig {
    size_t queue_capacity;
};

using MsgRoutingPath = boost::container::static_vector<IDataFlowNode*, 16>;

class Engine : public virtual IEngine {
  public:
    Engine(const ::nlohmann::json& json_config);

    void run() override;
    void emit(Msg* msg) override;

  private:
    void do_dfs(IDataFlowNode* current, MsgRoutingPath& path, Msg* msg);

  private:
    std::vector<ISourceNode*> _sources;
    std::vector<ISinkNode*> _sinks;
    std::vector<IPipe*> _pipes;
    std::vector<IFilter*> _filters;
    boost::sync_bounded_queue<Msg*> _msg_queue;
    const EngineConfig _config;

  private:
    std::map<std::string_view, const ISourceProvider*> _source_providers;
    std::map<std::string_view, const ISinkProvider*> _sink_providers;
    std::map<std::string_view, const IFilterProvider*> _filter_providers;
};

}; // namespace edgelink