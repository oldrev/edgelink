#include "edgelink/edgelink.hpp"
#include "edgelink/flow/dependency-sorter.hpp"
#include "edgelink/flow/details/flow-factory.hpp"

using namespace edgelink;
using namespace boost;
using namespace edgelink;

namespace edgelink::flow::details {

FlowFactory::FlowFactory(const IRegistry& registry) : _registry(registry) {}

std::vector<std::unique_ptr<IFlow>> FlowFactory::create_flows(const boost::json::array& flows_config) const {
    auto node_provider_type = rttr::type::get<INodeProvider>();

    // 这里注册测试用的
    // auto dataflow_elements = flows_config["dataflow"];
    std::vector<std::unique_ptr<IFlow>> flows;
    for (const auto& json_node_value : flows_config) {
        const auto& json_node = json_node_value.as_object();
        const std::string type(json_node.at("type").as_string());
        if (type == "tab") {
            auto flow = this->create_flow(flows_config, json_node);
            flows.emplace_back(std::move(flow));
        }
    }
    return flows;
}

std::unique_ptr<IFlow> FlowFactory::create_flow(const boost::json::array& flows_config,
                                                const boost::json::object& flow_node) const {

    // 创建边连接
    DependencySorter<boost::json::string> sorter;

    auto flow_node_id = flow_node.at("id").as_string();
    // 创建一个空的流
    auto flow = std::make_unique<Flow>(flow_node);

    // 提取属于指定流节点的下级节点
    std::map<const boost::json::string, const boost::json::object*> json_nodes;
    for (const auto& elem_value : flows_config) {
        const auto& elem = elem_value.as_object();
        if (!elem.contains("z")) {
            continue;
        }
        const auto& z = elem.at("z").as_string();
        if (z == flow_node_id) {
            const auto& node_id = elem.at("id").as_string();
            json_nodes[node_id] = &elem;
            for (const auto& port : elem.at("wires").as_array()) {
                for (const auto& endpoint : port.as_array()) {
                    sorter.add_edge(node_id, endpoint.as_string());
                }
            }
        }
    }

    // 先把 json 节点提取出来

    auto sorted_ids = sorter.sort();

    std::map<const boost::json::string, IFlowNode*> node_map;

    for (FlowNodeID i = 0; i < static_cast<FlowNodeID>(sorted_ids.size()); i++) {
        const auto& elem_id = sorted_ids[i];
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

        auto const& provider_iter = _registry.get_node_provider(elem_type);
        auto node = provider_iter->create(i, elem, std::move(ports), flow.get());
        spdlog::info("已开始创建数据流节点：[type='{0}', str_id='{1}', id={2}]", elem_type, elem_id, node->id());
        node_map[elem_id] = node.get();
        flow->emplace_node(std::move(node));
    }
    std::unique_ptr<IFlow> ret = std::move(flow);

    return ret;
}

}; // namespace edgelink::flow::details