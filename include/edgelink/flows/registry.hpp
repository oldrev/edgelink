#pragma once

namespace edgelink {

/// @brief 插件注册管理器
struct EDGELINK_EXPORT IRegistry {
    //
    virtual const std::unique_ptr<IFlowNodeProvider>& get_flow_node_provider(const std::string_view& name) const = 0;

    virtual const std::unique_ptr<IStandaloneNodeProvider>&
    get_standalone_node_provider(const std::string_view& name) const = 0;

#if EL_TEST
  private:
    RTTR_ENABLE()
#endif
};

class Registry : public IRegistry, private Noncopyable {
  public:
    Registry(const EdgeLinkSettings& el_config);
    virtual ~Registry();

    const std::unique_ptr<IFlowNodeProvider>& get_flow_node_provider(const std::string_view& type_name) const override;

    const std::unique_ptr<IStandaloneNodeProvider>&
    get_standalone_node_provider(const std::string_view& type_name) const override;

  private:
    void register_node_provider(const rttr::type& provider_type);

  private:
    std::shared_ptr<spdlog::logger> _logger;
    std::unordered_map<std::string_view, std::unique_ptr<IFlowNodeProvider>> _flow_node_providers;
    std::unordered_map<std::string_view, std::unique_ptr<IStandaloneNodeProvider>> _standalone_node_providers;
    std::vector<std::unique_ptr<rttr::library>> _libs;

#if EL_TEST
  private:
    RTTR_ENABLE(IRegistry)
#endif
};

}; // namespace edgelink
