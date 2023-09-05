#pragma once

namespace edgelink {

struct IFlowNode;

struct EngineConfig {};

using MsgRoutingPath = boost::container::static_vector<const IFlowNode*, 32>;

struct EdgeLinkConfig;

class Engine : public IEngine {
  public:
    Engine(const EdgeLinkConfig& el_config, const IFlowFactory& flow_factory);
    virtual ~Engine();

    bool is_disabled() const override { return _disabled; }

    Awaitable<void> start_async() override;
    Awaitable<void> stop_async() override;

    inline IFlow* get_flow(const std::string_view flow_id) const override {
        for (auto& flow : _flows) {
            if (flow->id() == flow_id) {
                return flow.get();
            }
        }
        return nullptr;
    }

  private:
    const IFlowFactory& _flow_factory;
    const std::string _flows_json_path;
    std::vector<std::unique_ptr<IFlowNode>> _nodes;

    std::unique_ptr<std::stop_source> _stop_source;
    const std::string _id;
    const std::string _name;
    bool _disabled;
    std::vector<std::unique_ptr<IFlow>> _flows;
};

}; // namespace edgelink