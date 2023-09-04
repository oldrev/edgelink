#include "edgelink/edgelink.hpp"
#include "edgelink/flow/dependency-sorter.hpp"

using namespace boost;
namespace this_coro = boost::asio::this_coro;

using CloneMsgStaticVector = boost::container::static_vector<std::shared_ptr<edgelink::Msg>, 32>;

namespace edgelink {

Engine::Engine(const nlohmann::json& json_config, const IRegistry& registry) : _config{}, _msg_id_counter(0) {

    auto node_provider_type = rttr::type::get<INodeProvider>();

    // 这里注册测试用的
    auto dataflow_elements = json_config["dataflow"];

    // 创建边连接
    DependencySorter<std::string> sorter;

    // 先把 json 节点提取出来
    std::map<const std::string, const nlohmann::json*> json_nodes;
    for (const auto& elem : dataflow_elements) {
        const std::string& node_id = elem.at("id");
        json_nodes[node_id] = &elem;
        for (const auto& port : elem.at("wires")) {
            for (const std::string& endpoint : port) {
                sorter.add_edge(node_id, endpoint);
            }
        }
    }
    auto sorted_ids = sorter.sort();

    // 第一遍扫描先创建节点
    std::map<const std::string_view, FlowNode*> node_map;

    for (uint32_t i = 0; i < static_cast<uint32_t>(sorted_ids.size()); i++) {
        const std::string& elem_id = sorted_ids[i];
        const nlohmann::json& elem = *json_nodes.at(elem_id);
        const std::string elem_type = elem.at("type");

        auto ports = std::vector<OutputPort>();
        for (const auto& port_config : elem.at("wires")) {
            auto output_wires = std::vector<FlowNode*>();
            for (const std::string& endpoint : port_config) {
                auto out_node = node_map.at(endpoint);
                output_wires.push_back(out_node);
            }
            auto port = OutputPort(std::move(output_wires));
            ports.emplace_back(std::move(port));
        }

        auto const& provider_iter = registry.get_node_provider(elem_type);
        auto node = provider_iter->create(i, elem, std::move(ports), this);
        spdlog::info("已开始创建数据流节点：[type='{0}', key='{1}', id={2}]", elem_type, elem_id, node->id());
        node_map[elem_id] = node.get();
        _nodes.emplace_back(std::move(node));
    }
}

Engine::~Engine() {
    //
    spdlog::info("数据流引擎关闭中...");
}

Awaitable<void> Engine::emit_async(uint32_t source_node_id, std::shared_ptr<Msg> msg) {
    //
    auto source = this->get_node(source_node_id);
    auto output_ports = source->output_ports();
    for (size_t i = 0; i < output_ports.size(); i++) {
        co_await this->relay_async(source_node_id, msg, i, true);
        // 根据出度把消息复制
    }
}

Awaitable<void> Engine::start_async() {
    //
    spdlog::info("开始启动数据流引擎");
    _stop_source = std::make_unique<std::stop_source>();
    spdlog::info("数据流引擎已启动");

    for (auto const& node : _nodes) {
        spdlog::info("正在启动数据流节点：{0}", node->descriptor()->type_name());
        co_await node->start_async();
        spdlog::info("数据流节点 '{0}' 已启动", node->descriptor()->type_name());
    }
    spdlog::info("全部节点启动完毕");
}

Awaitable<void> Engine::stop_async() {
    // 给出线程池停止信号
    spdlog::info("开始请求数据流引擎线程池停止...");
    _stop_source->request_stop();

    spdlog::info("数据流引擎线程池已停止");
    co_return;
}

Awaitable<void> Engine::relay_async(uint32_t source_node_id, std::shared_ptr<Msg> orig_msg, size_t port,
                                    bool clone) const {
    auto source = this->get_node(source_node_id);
    // 根据出度把消息复制
    auto output_ports = source->output_ports();

    auto output_port = output_ports.at(port);
    for (auto j = 0; j < output_port.wires().size(); j++) {
        auto endpoint = output_port.wires().at(j);
        auto msg = clone && j > 0 ? std::make_shared<Msg>(*orig_msg) : orig_msg;

        // 线程池中处理数据流
        //
        switch (endpoint->descriptor()->kind()) {

        case NodeKind::FILTER:
        case NodeKind::SINK:
        case NodeKind::JUNCTION: {
            co_await endpoint->receive_async(msg);
        } break;

        default:
            throw InvalidDataException("错误的节点连线");
        }
    }
    co_return;
}

}; // namespace edgelink