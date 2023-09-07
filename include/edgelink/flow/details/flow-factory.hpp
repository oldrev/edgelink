#pragma once

namespace edgelink {

struct IStandaloneNode;
struct IFlow;
struct IFlowNode;

}; // namespace edgelink

namespace edgelink::flow::details {

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
    const IRegistry& _registry;
};

}; // namespace edgelink::flow::details