#include "edgelink/edgelink.hpp"
#include "edgelink/flows/dependency-sorter.hpp"
#include "flow.hpp"
#include "flow-factory.hpp"

using namespace edgelink;
using namespace boost;
using namespace edgelink;

namespace edgelink::flows {

FlowFactory::FlowFactory(const IRegistry& registry)
    : _logger(spdlog::default_logger()->clone("Flow")), _registry(registry) {
    //
}

std::vector<std::unique_ptr<IFlow>> FlowFactory::create_flows(const boost::json::array& flows_config,
                                                              IEngine* engine) const {
    auto node_provider_type = rttr::type::get<IFlowNodeProvider>();

    // auto dataflow_elements = flows_config["dataflow"];
    std::vector<std::unique_ptr<IFlow>> flows;
    for (const auto& json_node_value : flows_config) {
        const auto& json_node = json_node_value.as_object();
        const std::string type(json_node.at("type").as_string());
        if (type == "tab" || type == "flow") {
            try {
                auto flow = this->create_flow(flows_config, json_node, engine);
                flows.emplace_back(std::move(flow));
            } catch (std::exception& ex) {
                _logger->error("创建流时发生错误：{}", ex.what());
                throw;
            }
        }
    }
    return flows;
}

std::vector<std::unique_ptr<IStandaloneNode>> FlowFactory::create_global_nodes(const boost::json::array& flows_config,
                                                                               IEngine* engine) const {
    // 创建全局节点
    std::vector<std::unique_ptr<IStandaloneNode>> global_nodes;
    for (const auto& json_node_value : flows_config) {
        const auto& json_node = json_node_value.as_object();
        const std::string_view elem_type = json_node.at("type").as_string();
        const std::string_view elem_id = json_node.at("id").as_string();
        if (elem_type != "tab" && elem_type != "flow" && !json_node.contains("z")) {
            auto const& provider_iter = _registry.get_standalone_node_provider(elem_type);
            _logger->info("开始创建独立节点：[type='{0}', id='{1}']", elem_type, elem_id);
            try {
                auto node = provider_iter->create(elem_id, json_node, engine);
                global_nodes.emplace_back(std::move(node));
            } catch (std::exception& ex) {
                _logger->error("开始创建独立节点：[type='{}', id='{}'] 发生错误：{}", elem_type, elem_id, ex.what());
                throw;
            }
        }
    }

    return global_nodes;
}

std::unique_ptr<IFlow> FlowFactory::create_flow(const boost::json::array& flows_config,
                                                const boost::json::object& flow_node, IEngine* engine) const {

    // 创建边连接
    DependencySorter<std::string_view> sorter;

    auto flow_node_id = flow_node.at("id").as_string();
    // 创建一个空的流
    auto flow = std::make_unique<Flow>(flow_node, engine);

    // 提取属于指定流节点的下级节点
    std::map<const std::string_view, const boost::json::object*> json_nodes;
    for (const auto& elem_value : flows_config) {
        const auto& elem = elem_value.as_object();
        const auto& elem_type = elem.at("type").as_string();

        // 跳过全局节点和注释
        if (!elem.contains("z") || elem_type == "comment") {
            continue;
        }
        const auto& z = elem.at("z").as_string();
        if (z == flow_node_id) {
            const auto& node_id = elem.at("id").as_string();
            json_nodes[node_id] = &elem;
            for (const auto& port : elem.at("wires").as_array()) {
                for (const auto& endpoint : port.as_array()) {
                    std::string_view from = node_id;
                    std::string_view to = endpoint.as_string();
                    sorter.add_edge(from, to);
                }
            }
        }
    }

    // 先把 json 节点提取出来

    auto sorted_ids = sorter.sort();

    std::map<const std::string_view, IFlowNode*> node_map;

    for (size_t i = 0; i < sorted_ids.size(); i++) {
        const std::string_view elem_id = sorted_ids[i];
        const boost::json::object& elem = *json_nodes.at(elem_id);
        const auto& elem_type = elem.at("type").as_string();

        auto ports = std::vector<OutputPort>();
        for (const auto& port_config : elem.at("wires").as_array()) {
            auto output_wires = std::vector<IFlowNode*>();
            for (const auto& endpoint : port_config.as_array()) {
                auto out_node = node_map.at(endpoint.as_string());
                output_wires.push_back(out_node);
            }
            auto port = OutputPort(std::move(output_wires));
            ports.emplace_back(std::move(port));
        }

        _logger->info("开始创建流程节点：[type='{0}', json_id='{1}']", elem_type, elem_id);
        auto const& provider_iter = _registry.get_flow_node_provider(elem_type);
        try {
            auto node = provider_iter->create(elem_id, elem, std::move(ports), flow.get());
            node_map[elem_id] = node.get();
            flow->emplace_node(std::move(node));
        } catch (std::exception& ex) {
            _logger->error("开始创建独立节点：[type='{}', id='{}'] 发生错误：{}", elem_type, elem_id, ex.what());
            throw;
        }
    }
    std::unique_ptr<IFlow> ret = std::move(flow);

    return ret;
}

RTTR_REGISTRATION {
    rttr::registration::class_<edgelink::IFlowFactory>("edgelink::flow::IFlowFactory");
    rttr::registration::class_<edgelink::flows::FlowFactory>("edgelink::flows::FlowFactory");
}

}; // namespace edgelink::flows