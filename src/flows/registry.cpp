#include "edgelink/edgelink.hpp"

namespace edgelink {

bool is_valid_provider_type(const rttr::type& type) {
    return (type.is_derived_from<IFlowNodeProvider>() || type.is_derived_from<IStandaloneNodeProvider>()) &&
           !type.is_pointer() && type.is_class() && type.get_name() != "edgelink::IStandaloneNodeProvider" &&
           type.get_name() != "edgelink::IFlowNodeProvider";
}

Registry::Registry(const boost::json::object& json_config) : _logger(spdlog::default_logger()->clone("Flow")), _libs() {

    auto node_provider_type = rttr::type::get<INodeProvider>();

    // 注册内置节点
    {
        _logger->info("开始注册内置流程节点...");
        // 注册节点提供器
        for (auto& type : rttr::type::get_types()) {
            if (is_valid_provider_type(type)) {
                this->register_node_provider(type);
            }
        }
    }

    _logger->info("开始注册插件提供的流程节点...");
    std::string path = "./plugins";

    using std::filesystem::directory_iterator;

    for (const auto& file : directory_iterator(path)) {
        auto path = std::filesystem::path(file.path());
        std::string lib_path = path; // path.replace_extension("");
        _logger->info("找到插件：{}", lib_path);

        auto lib = make_unique<rttr::library>(lib_path);
        auto is_loaded = lib->load();
        if (!is_loaded) {
            throw std::runtime_error(
                fmt::format("无法加载插件 '{}'：{}", lib_path, std::string(lib->get_error_string())));
        }

        for (auto type : lib->get_types()) {
            if (is_valid_provider_type(type)) {
                _logger->debug("发现插件节点类型：{}", std::string(type.get_name()));
                this->register_node_provider(type);
            }
        }
        // 把插件也注册进去
        _libs.emplace_back(std::move(lib));
    }
}

Registry::~Registry() {
    for (auto const& lib : _libs) {
        _logger->info("开始卸载插件动态库：{}", std::string(lib->get_file_name()));
        lib->unload();
    }
}

const std::unique_ptr<IFlowNodeProvider>& Registry::get_flow_node_provider(const std::string_view& type_name) const {
    if (!_flow_node_providers.contains(type_name)) {
        _logger->error("找不到流程节点类型提供器：type={}", type_name);
    }
    return _flow_node_providers.at(type_name);
}

const std::unique_ptr<IStandaloneNodeProvider>&
Registry::get_standalone_node_provider(const std::string_view& type_name) const {
    if (!_standalone_node_providers.contains(type_name)) {
        _logger->error("找不到独立节点类型提供器：type={}", type_name);
    }
    return _standalone_node_providers.at(type_name);
}

void Registry::register_node_provider(const rttr::type& provider_type) {

    auto flow_node_provider_type = rttr::type::get<IFlowNodeProvider>();
    auto standalone_node_provider_type = rttr::type::get<IStandaloneNodeProvider>();

    if (provider_type.is_derived_from(flow_node_provider_type)) {
        auto provider = provider_type.create().get_value<IFlowNodeProvider*>();
        auto desc = provider->descriptor();
        _logger->info("注册流程节点提供器: '{}'", desc->type_name());
        _flow_node_providers.emplace(desc->type_name(), std::move(provider));
    } else if (provider_type.is_derived_from(standalone_node_provider_type)) {
        auto provider = provider_type.create().get_value<IStandaloneNodeProvider*>();
        auto desc = provider->descriptor();
        _logger->info("注册流程节点提供器: '{}'", desc->type_name());
        _standalone_node_providers.emplace(desc->type_name(), std::move(provider));
    } else {
        _logger->error("未知的节点提供器: '{}'", std::string(provider_type.get_name()));
    }
}

}; // namespace edgelink