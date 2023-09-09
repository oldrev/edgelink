#pragma once

namespace edgelink {

struct IRegistry {
    //
    virtual const std::unique_ptr<IFlowNodeProvider>& get_flow_node_provider(const std::string_view& name) const = 0;

    virtual const std::unique_ptr<IStandaloneNodeProvider>&
    get_standalone_node_provider(const std::string_view& name) const = 0;
};

class Registry : public IRegistry {
  public:
    Registry(const boost::json::object& json_config);
    virtual ~Registry();

    inline const std::unique_ptr<IFlowNodeProvider>&
    get_flow_node_provider(const std::string_view& name) const override {
        return _flow_node_providers.at(name);
    }

    inline const std::unique_ptr<IStandaloneNodeProvider>&
    get_standalone_node_provider(const std::string_view& name) const override {
        return _standalone_node_providers.at(name);
    }

  private:
    void register_node_provider(const rttr::type& provider_type);

  private:
    std::shared_ptr<spdlog::logger> _logger;
    std::unordered_map<std::string_view, std::unique_ptr<IFlowNodeProvider>> _flow_node_providers;
    std::unordered_map<std::string_view, std::unique_ptr<IStandaloneNodeProvider>> _standalone_node_providers;
    std::vector<std::unique_ptr<rttr::library>> _libs;
};

}; // namespace edgelink