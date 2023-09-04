#pragma once

namespace edgelink::flow::details {

class FlowFactory : public edgelink::IFlowFactory {
  public:
    FlowFactory(const IRegistry& registry);

    std::vector<std::unique_ptr<edgelink::IFlow>> create_flows(const nlohmann::json& flows_config);

  private:
    std::unique_ptr<edgelink::IFlow> create_flow(const nlohmann::json& flows_config, const nlohmann::json& flow_node);

  private:
    const IRegistry& _registry;
};

}; // namespace edgelink::flow::details