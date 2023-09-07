#include "edgelink/edgelink.hpp"

namespace edgelink {

Registry::Registry(const boost::json::object& json_config) : _libs() {

    auto node_provider_type = rttr::type::get<INodeProvider>();

    // 注册内置节点
    {
        spdlog::info("开始注册内置流程节点...");
        // 注册节点提供器
        auto node_providers = node_provider_type.get_derived_classes();
        for (auto& pt : node_providers) {
            this->register_node_provider(pt);
        }
    }

    spdlog::info("开始注册插件提供的流程节点...");
    std::string path = "./plugins";

    using std::filesystem::directory_iterator;

    for (const auto& file : directory_iterator(path)) {
        auto path = std::filesystem::path(file.path());
        std::string lib_path = path.replace_extension("");
        spdlog::info("找到插件：{0}", lib_path);

        auto lib = make_unique<rttr::library>(lib_path);
        if (!lib->load()) {
            throw std::runtime_error(fmt::format("无法加载插件 {0}", lib_path));
        }

        for (auto type : lib->get_types()) {
            if ((type.is_derived_from<IFlowNodeProvider>() || type.is_derived_from<IStandaloneNodeProvider>()) && !type.is_pointer() && type.is_class()) {
                this->register_node_provider(type);
            }
        }
        // 把插件也注册进去
        _libs.emplace_back(std::move(lib));
    }
}

Registry::~Registry() {
    for (auto const& lib : _libs) {
        spdlog::info("开始卸载插件动态库：{0}", lib->get_file_name());
        lib->unload();
    }
}

void Registry::register_node_provider(const rttr::type& provider_type) {

    auto flow_node_provider_type = rttr::type::get<IFlowNodeProvider>();
    auto standalone_node_provider_type = rttr::type::get<IStandaloneNodeProvider>();


    if (provider_type.is_derived_from(flow_node_provider_type)) {
        auto node_provider = provider_type.create().get_value<INodeProvider*>();
        auto provider = rttr::rttr_cast<IFlowNodeProvider*>(node_provider);
        auto desc = provider->descriptor();
        spdlog::info("注册流程节点提供器: [{0}]", desc->type_name());
        _flow_node_providers.emplace(desc->type_name(), std::move(provider));
    } else if (provider_type.is_derived_from(standalone_node_provider_type)) {
        auto node_provider = provider_type.create().get_value<INodeProvider*>();
        auto provider = rttr::rttr_cast<IStandaloneNodeProvider*>(node_provider);
        auto desc = provider->descriptor();
        spdlog::info("注册流程节点提供器: [{0}]", desc->type_name());
        _standalone_node_providers.emplace(desc->type_name(), std::move(provider));
    } else {
        spdlog::error("未知的节点提供器: [{0}]", provider_type.get_name());
    }
}

}; // namespace edgelink