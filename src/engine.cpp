#include "pch.hpp"

#include "edgelink/edgelink.hpp"

using namespace std;

using PipeStaticVector = boost::container::static_vector<const edgelink::IPipe*, 128>;

namespace edgelink {


Engine::Engine(const nlohmann::json& json_config)
    : _config{.queue_capacity = 100}, _msg_queue(boost::sync_bounded_queue<Msg*>(100)) {

    // 注册 nodes
    spdlog::info("开始注册数据流节点");
    auto node_provider_type = rttr::type::get<INodeProvider>();
    auto node_providers = node_provider_type.get_derived_classes();
    for (auto& pt : node_providers) {
        auto provider_var = pt.create();
        auto provider = provider_var.get_value<INodeProvider*>();
        auto desc = provider->descriptor();
        _node_providers[desc->type_name()] = provider;
        spdlog::info("注册数据流节点: [class_name={0}, type_name={1}]", pt.get_name(), desc->type_name());
    }

    // 这里注册测试用的
    auto dataflow_elements = json_config["dataflow"];

    // 第一遍扫描先创建节点
    std::map<std::string, IDataFlowNode*> node_map;

    for (const auto& elem : dataflow_elements) {
        const std::string elem_type = elem["$type"];
        if (elem_type == "pipe") { // 管道直接跳过
            continue;
        }

        const std::string elem_key = elem["@key"];
        spdlog::info("开始创建数据流节点：[$type={0}, @key={1}]", elem_type, elem_key);
        auto provider_iter = _node_providers.find(elem_type);
        if(provider_iter ==_node_providers.end()) {
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
        if (elem_type != "pipe") { // 非管道就跳过
            continue;
        }
        spdlog::info("开始创建数据流管道");
        const std::string& input_key = elem["@input"];
        const std::string& output_key = elem["@output"];
        auto input_node = node_map.at(input_key);
        auto output_node = node_map.at(output_key);
        auto pipe = new ForwardPipe(input_node, output_node);
        _pipes.push_back(pipe);
        spdlog::info("已创建数据流管道");
    }
}

Engine::~Engine() {

    for (auto pipe : _pipes) {
        delete pipe;
    }

    for (auto node : _nodes) {
        delete node;
    }
}

void Engine::emit(Msg* msg) {
    // 处理消息
    // 这里只是概念验证原型
    // 消息来源调用此函数将消息存入队列，然后引擎 worker 线程取出消息进入流水线处理

    _msg_queue.wait_push_back(msg);
}

void Engine::run() {

    /*
    vector<thread> threads;

    // 创建并启动多个线程
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back(threadFunction, i + 1);
    }

    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    */

    /*
    for (auto& i : _filters) {
        spdlog::info("正在启动过滤器：[type={0}]", i.first);
        i.second->start();
    }
    */

    for (auto node : _nodes) {

        spdlog::info("正在启动数据源节点：{0}", typeid(node).name());
        auto source_node = dynamic_cast<ISourceNode*>(node); 
        if(source_node != nullptr) {
            source_node->start();
        }
    }
    spdlog::info("全部节点启动完毕");

    // 引擎主线程
    spdlog::info("正在启动引擎工作线程");
    auto thread = std::jthread([this](std::stop_token stoken) { this->worker_proc(stoken); });
    spdlog::info("引擎工作线程已启动");
    thread.join();
}

void Engine::worker_proc(std::stop_token stoken) {
    while (!stoken.stop_requested()) {

        Msg* msg;
        try {
            _msg_queue.wait_pull_front(msg);

            MsgRoutingPath path;
            this->do_dfs(msg->source, path, msg);
        } catch (std::exception& ex) {
            spdlog::error("处理消息时发生了异常: {0}", ex.what());
        } catch (...) {
            spdlog::error("处理消息时发生了未知异常");
        }

        // 消息处理完就删掉
        delete msg;
    }
}

void Engine::do_dfs(IDataFlowNode* current, MsgRoutingPath& path, Msg* msg) {

    // 将当前节点添加到路径中
    path.push_back(current);

    PipeStaticVector out_pipes;
    // 找到以当前节点为起点的所有边
    for (auto pipe : _pipes) {
        if (pipe->input() == current) {
            out_pipes.push_back(pipe);
        }
    }

    for (size_t i = 0; i < out_pipes.size(); i++) {
        const auto pipe  = out_pipes[i]; 

        // 检查目标节点是否已经在路径中，以避免循环
        bool is_visited = false;
        for (auto dest_node : path) {
            if (dest_node == pipe->output()) {
                is_visited = true;
                break;
            }
        }

        if (!is_visited) {

            // TODO FIXME 把这些 dynamic_cast 替换掉
            auto target_sink_node = dynamic_cast<ISinkNode*>(pipe->output());
            if (target_sink_node != nullptr) { // 遇到了收集器就停止了
                // auto new_msg = i > 0 ? msg->clone() : msg;
                target_sink_node->receive(msg);
            } else { // 其他只可能是过滤器节点
                auto target_filter_node = dynamic_cast<IFilter*>(pipe->output());
                if (target_filter_node == nullptr) {
                    throw InvalidDataException("配置错误，Pipe 指向了了非 IFilter 或 ISinkNode 节点");
                }
                // 执行过滤器
                target_filter_node->filter(msg);

                // 递归调用DFS来继续探索路径
                this->do_dfs(pipe->output(), path, msg);
            }
        }
    }

    // 打印路径或执行其他操作
    /*
    if (path.size() > 1) {
        std::cout << "Path: ";
        std::cout << std::endl;
    }
    */

    // 从路径中移除当前节点，以回溯到之前的节点
    path.pop_back();
}

}; // namespace edgelink