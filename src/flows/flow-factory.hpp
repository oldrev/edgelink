#pragma once

namespace edgelink {

struct IStandaloneNode;
struct IFlow;
struct IFlowNode;

}; // namespace edgelink

namespace edgelink::flows {

class FlowFactory : public edgelink::IFlowFactory {
  public:
    FlowFactory(const IRegistry& registry);

    std::vector<std::unique_ptr<IFlow>> create_flows(const boost::json::array& flows_config, IEngine* engine) const override;

    std::vector<std::unique_ptr<edgelink::IStandaloneNode>> create_global_nodes(const boost::json::array& flows_config,
                                                                                IEngine* engine) const override;

  private:
    std::unique_ptr<edgelink::IFlow> create_flow(const boost::json::array& flows_config,
                                                 const boost::json::object& flow_node, IEngine* engine) const;

  private:
    std::shared_ptr<spdlog::logger> _logger;
    const IRegistry& _registry;

#if EL_TEST
private:
    RTTR_ENABLE(IFlowFactory)
#endif
};

}; // namespace edgelink::flows