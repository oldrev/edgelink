#pragma once

namespace edgelink::flow::details {

class FlowFactory : public edgelink::IFlowFactory {
  public:
    FlowFactory(const IRegistry& registry);

    std::vector<std::unique_ptr<edgelink::IFlow>> create_flows(const boost::json::array& flows_config);

  private:
    std::unique_ptr<edgelink::IFlow> create_flow(const boost::json::array& flows_config,
                                                 const boost::json::object& flow_node);

  private:
    const IRegistry& _registry;
};

}; // namespace edgelink::flow::details