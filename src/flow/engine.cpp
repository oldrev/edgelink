#include "../pch.hpp"

#include "edgelink/edgelink.hpp"
#include "edgelink/flow/dependency-sorter.hpp"

using namespace std;

using CloneMsgStaticVector = boost::container::static_vector<std::shared_ptr<edgelink::Msg>, 32>;

namespace edgelink {

Engine::Engine(const nlohmann::json& json_config) : _config{}, _msg_id_counter(0) {

    {
        // 注册节点提供器
        spdlog::info("开始注册数据流节点");
        auto node_provider_type = rttr::type::get<INodeProvider>();
        auto node_providers = node_provider_type.get_derived_classes();
        for (auto& pt : node_providers) {
            auto provider_var = pt.create();
            auto provider = provider_var.get_value<INodeProvider*>();
            auto desc = provider->descriptor();
            _node_providers[desc->type_name()] = provider;
            spdlog::info("注册数据流节点: [class_name='{0}', type_name='{1}']", pt.get_name(), desc->type_name());
        }
    }

    // 这里注册测试用的
    auto dataflow_elements = json_config["dataflow"];

    // 创建边连接
    DependencySorter<string> sorter;

    // 先把 json 节点提取出来
    map<const string, const nlohmann::json*> json_nodes;
    for (const auto& elem : dataflow_elements) {
        const string& node_key = elem.at("key");
        json_nodes[node_key] = &elem;
        for (const auto& port : elem.at("wires")) {
            for (const string& endpoint : port) {
                sorter.add_edge(node_key, endpoint);
            }
        }
    }
    auto sorted_keys = sorter.sort();

    // 第一遍扫描先创建节点
    std::map<const string_view, FlowNode*> node_map;

    for (uint32_t i = 0; i < static_cast<uint32_t>(sorted_keys.size()); i++) {
        const string& elem_key = sorted_keys[i];
        const nlohmann::json& elem = *json_nodes.at(elem_key);
        const std::string elem_type = elem.at("$type");

        spdlog::info("开始创建数据流节点：[$type='{0}', key='{1}']", elem_type, elem_key);
        auto ports = vector<OutputPort>();
        for (const auto& port : elem.at("wires")) {
            auto output_wires = vector<FlowNode*>();
            for (const string& endpoint : port) {
                auto out_node = node_map.at(endpoint);
                output_wires.push_back(out_node);
            }
            ports.push_back(OutputPort(output_wires));
        }

        auto provider_iter = _node_providers.find(elem_type);
        if (provider_iter == _node_providers.end()) {
            spdlog::error("找不到数据流节点配型：'{0}'", elem_type);
            throw BadConfigException(elem_type, "无效的配置主键");
        }
        auto node = provider_iter->second->create(i, elem, ports, this);
        _nodes.push_back(node);
        node_map[elem_key] = node;
    }

}

Engine::~Engine() {

    for (auto node : _nodes) {
        delete node;
    }
}

void Engine::emit(shared_ptr<Msg> msg) {
    //
    this->relay(msg->source, msg); }

void Engine::start() {
    //
    spdlog::info("开始启动数据流引擎");
    _stop_source = make_unique<std::stop_source>();
    _pool = make_unique<boost::asio::thread_pool>(4);
    spdlog::info("数据流引擎已启动");

    for (auto node : _nodes) {
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

void Engine::relay(const FlowNode* source, const std::shared_ptr<Msg>& orig_msg, bool clone) const {

    // 根据出度把消息复制
    CloneMsgStaticVector out_msgs;
    out_msgs.push_back(orig_msg);

    auto output_ports = source->output_ports();

    for (auto i = 0; i < output_ports.size(); i++) {
        auto port = output_ports.at(i);
        for (auto j = 0; j < port.wires().size(); j++) {
            auto endpoint = port.wires().at(i);
            auto k = i * j;
            auto msg = clone && k > 0 ? make_shared<Msg>(*orig_msg) : orig_msg;

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
}

}; // namespace edgelink