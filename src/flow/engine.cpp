#include "../pch.hpp"

#include "edgelink/edgelink.hpp"

using namespace std;

using WireStaticVector = boost::container::static_vector<const edgelink::Wire*, 32>;
using CloneMsgStaticVector = boost::container::static_vector<std::shared_ptr<edgelink::Msg>, 32>;

namespace edgelink {

Engine::Engine(const nlohmann::json& json_config) : _config{}, _msg_id_counter(0) {

    // 注册 nodes
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

    // 这里注册测试用的
    auto dataflow_elements = json_config["dataflow"];

    // 第一遍扫描先创建节点
    std::map<std::string, FlowNode*> node_map;

    for (const auto& elem : dataflow_elements) {
        const std::string elem_type = elem["$type"];
        if (elem_type == "wire") { // 管道直接跳过
            continue;
        }

        const std::string elem_key = elem["key"];
        spdlog::info("开始创建数据流节点：[$type='{0}', key='{1}']", elem_type, elem_key);
        auto provider_iter = _node_providers.find(elem_type);
        if (provider_iter == _node_providers.end()) {
            spdlog::error("找不到数据流节点配型：'{0}'", elem_type);
            throw BadConfigException(elem_type, "无效的配置主键");
        }
        auto node = provider_iter->second->create(elem, this);
        _nodes.push_back(node);
        node_map[elem_key] = node;
    }

    // 第二遍扫描创建管道
    for (const auto& elem : dataflow_elements) {
        const std::string elem_type = elem["$type"];
        if (elem_type != "wire") { // 非管道就跳过
            continue;
        }
        spdlog::info("开始创建数据流管道");
        const std::string& input_key = elem["input"];
        const std::string& output_key = elem["output"];
        auto input_node = node_map.at(input_key);
        auto output_node = node_map.at(output_key);
        auto wire = new Wire(input_node, output_node);
        _wires.push_back(wire);
        _node_wires[input_node].push_back(wire);
        spdlog::info("已创建数据流管道");
    }
}

Engine::~Engine() {

    for (auto wire : _wires) {
        delete wire;
    }

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

void Engine::relay(const FlowNode* source, std::shared_ptr<Msg> orig_msg, bool clone) const {

    // 根据出度把消息复制
    CloneMsgStaticVector out_msgs;
    out_msgs.push_back(orig_msg);

    auto wires = source->wires();

    for (auto i = 1; i < wires.size(); i++) {
        if (clone) {
            auto new_msg = make_shared<Msg>(*orig_msg);
            out_msgs.push_back(new_msg);
        } else {
            out_msgs.push_back(orig_msg);
        }
    }

    for (size_t i = 0; i < wires.size(); i++) {
        const auto wire = wires[i];
        auto msg = out_msgs[i];

        // 线程池中处理数据流
        boost::asio::post(*_pool, [msg, this, wire]() {
            //
            switch (wire->output()->descriptor()->kind()) {

            case NodeKind::FILTER: {
                auto filter = static_cast<FilterNode*>(wire->output());
                filter->receive(msg);
            } break;

            case NodeKind::SINK: {
                auto sink = static_cast<SinkNode*>(wire->output());
                sink->receive(msg);
            } break;

            default:
                throw InvalidDataException("错误的节点连线");
            }
        });
    }
}

}; // namespace edgelink