#include "edgelink/edgelink.hpp"
#include "edgelink/flow/dependency-sorter.hpp"
#include "edgelink/flow/details/flow-factory.hpp"

using namespace edgelink;
using namespace boost;
using namespace edgelink;

namespace edgelink::flow::details {

FlowFactory::FlowFactory(const IRegistry& registry) : _registry(registry) {}

std::vector<std::unique_ptr<IFlow>> FlowFactory::create_flows(const nlohmann::json& flows_config) {
    auto node_provider_type = rttr::type::get<INodeProvider>();

    // 这里注册测试用的
    // auto dataflow_elements = flows_config["dataflow"];
    std::vector<std::unique_ptr<IFlow>> flows;
    for (auto json_node : flows_config) {
        const std::string& type = json_node.at("type");
        if (type == "tab") {
            auto flow = this->create_flow(flows_config, json_node);
            flows.emplace_back(std::move(flow));
        }
    }
    return flows;
}

std::unique_ptr<IFlow> FlowFactory::create_flow(const nlohmann::json& flows_config, const nlohmann::json& flow_node) {

    // 创建边连接
    DependencySorter<std::string> sorter;

    auto flow_node_id = flow_node.at("id");
    // 创建一个空的流
    auto flow = std::make_unique<Flow>(flow_node_id);

    // 提取属于指定流节点的下级节点
    std::map<const std::string, const nlohmann::json*> json_nodes;
    for (const auto& elem : flows_config) {
        const std::string z = elem.value("z", "");
        if (z == flow_node_id) {
            const std::string& node_id = elem.at("id");
            json_nodes[node_id] = &elem;
            for (const auto& port : elem.at("wires")) {
                for (const std::string& endpoint : port) {
                    sorter.add_edge(node_id, endpoint);
                }
            }
        }
    }

    // 先把 json 节点提取出来

    auto sorted_ids = sorter.sort();

    std::map<const std::string_view, IFlowNode*> node_map;

    for (uint32_t i = 0; i < static_cast<uint32_t>(sorted_ids.size()); i++) {
        const std::string& elem_id = sorted_ids[i];
        const nlohmann::json& elem = *json_nodes.at(elem_id);
        const std::string elem_type = elem.at("type");

        auto ports = std::vector<OutputPort>();
        for (const auto& port_config : elem.at("wires")) {
            auto output_wires = std::vector<IFlowNode*>();
            for (const std::string& endpoint : port_config) {
                auto out_node = node_map.at(endpoint);
                output_wires.push_back(out_node);
            }
            auto port = OutputPort(std::move(output_wires));
            ports.emplace_back(std::move(port));
        }

        auto const& provider_iter = _registry.get_node_provider(elem_type);
        auto node = provider_iter->create(i, elem, std::move(ports), flow.get());
        spdlog::info("已开始创建数据流节点：[type='{0}', key='{1}', id={2}]", elem_type, elem_id, node->id());
        node_map[elem_id] = node.get();
        flow->emplace_node(std::move(node));
    }
    std::unique_ptr<IFlow> ret = std::move(flow);

    return ret;
}

}; // namespace edgelink::flow::details