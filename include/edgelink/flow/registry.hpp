#pragma once

namespace edgelink {

struct IRegistry {
    //
    virtual const std::unique_ptr<IFlowNodeProvider>& get_node_provider(const std::string_view& name) const = 0;
};

class Registry : public IRegistry {
  public:
    Registry(const boost::json::object& json_config);
    virtual ~Registry();

    inline const std::unique_ptr<IFlowNodeProvider>& get_node_provider(const std::string_view& name) const override {
        return _node_providers.at(name);
    }

  private:
    void register_node_provider(const rttr::type& provider_type);

  private:
    std::unordered_map<std::string_view, std::unique_ptr<IFlowNodeProvider>> _node_providers;
    std::vector<std::unique_ptr<rttr::library>> _libs;
};

}; // namespace edgelink