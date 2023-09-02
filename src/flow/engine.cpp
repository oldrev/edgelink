#include "../pch.hpp"

#include "edgelink/edgelink.hpp"
#include "edgelink/flow/dependency-sorter.hpp"

using namespace std;

using CloneMsgStaticVector = boost::container::static_vector<std::shared_ptr<edgelink::Msg>, 32>;

namespace edgelink {

Engine::Engine(const nlohmann::json& json_config, const IRegistry& registry) : _config{}, _msg_id_counter(0) {

    auto node_provider_type = rttr::type::get<INodeProvider>();

    // 这里注册测试用的
    auto dataflow_elements = json_config["dataflow"];

    // 创建边连接
    DependencySorter<string> sorter;

    // 先把 json 节点提取出来
    map<const string, const nlohmann::json*> json_nodes;
    for (const auto& elem : dataflow_elements) {
        const string& node_id = elem.at("id");
        json_nodes[node_id] = &elem;
        for (const auto& port : elem.at("wires")) {
            for (const string& endpoint : port) {
                sorter.add_edge(node_id, endpoint);
            }
        }
    }
    auto sorted_ids = sorter.sort();

    // 第一遍扫描先创建节点
    std::map<const string_view, FlowNode*> node_map;

    for (uint32_t i = 0; i < static_cast<uint32_t>(sorted_ids.size()); i++) {
        const string& elem_id = sorted_ids[i];
        const nlohmann::json& elem = *json_nodes.at(elem_id);
        const std::string elem_type = elem.at("type");

        auto ports = vector<OutputPort>();
        for (const auto& port : elem.at("wires")) {
            auto output_wires = vector<FlowNode*>();
            for (const string& endpoint : port) {
                auto out_node = node_map.at(endpoint);
                output_wires.emplace_back(out_node);
            }
            ports.emplace_back(OutputPort(move(output_wires)));
        }

        auto const& provider_iter = registry.get_node_provider(elem_type);
        auto node = provider_iter->create(i, elem, std::move(ports), this);
        spdlog::info("已开始创建数据流节点：[type='{0}', key='{1}', id={2}]", elem_type, elem_id, node->id());
        node_map[elem_id] = node.get();
        _nodes.emplace_back(move(node));
    }
}

Engine::~Engine() {}

void Engine::emit(shared_ptr<Msg>& msg) {
    //
    this->relay(msg->birth_place, msg);
}

void Engine::start() {
    //
    spdlog::info("开始启动数据流引擎");
    _stop_source = make_unique<std::stop_source>();
    _pool = make_unique<boost::asio::thread_pool>(4);
    spdlog::info("数据流引擎已启动");

    for (auto const& node : _nodes) {
        spdlog::info("正在启动数据流节点：{0}", node->descriptor()->type_name());
        node->start();
    }
    spdlog::info("全部节点启动完毕");
}

void Engine::stop() {
    // 给出线程池停止信号
    spdlog::info("开始请求数据流引擎线程池停止...");
    _stop_source->request_stop();

    // 等待线程池停止
    _pool->join();
    spdlog::info("数据流引擎线程池已停止");
}

void Engine::run() {
    // 引擎主线程
    spdlog::info("正在启动引擎工作线程");
    auto thread = std::jthread([this]() {
        // 阻塞主线程
        while (!_stop_source->stop_requested()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    spdlog::info("引擎工作线程已启动");
    thread.join();
}

void Engine::relay(const FlowNode* source, const std::shared_ptr<Msg>& orig_msg, size_t port, bool clone) const {

    // 根据出度把消息复制
    auto output_ports = source->output_ports();

    auto output_port = output_ports.at(port);
    for (auto j = 0; j < output_port.wires().size(); j++) {
        auto endpoint = output_port.wires().at(j);
        auto msg = clone && j > 0 ? make_shared<Msg>(*orig_msg) : orig_msg;

        // 线程池中处理数据流
        boost::asio::post(*_pool, [msg, this, endpoint]() {
            //
            switch (endpoint->descriptor()->kind()) {

            case NodeKind::FILTER:
            case NodeKind::SINK:
            case NodeKind::JUNCTION: {
                endpoint->receive(msg);
            } break;

            default:
                throw InvalidDataException("错误的节点连线");
            }
        });
    }
}

}; // namespace edgelink