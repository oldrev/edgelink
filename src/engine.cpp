#include "pch.hpp"

#include "edgelink/edgelink.hpp"

using namespace std;

namespace edgelink {

Engine::Engine(const nlohmann::json& json_config)
    : _config{.queue_capacity = 100}, _msg_queue(boost::sync_bounded_queue<Msg*>(100)) {

    {
        // 注册 sources
        auto source_provider_type = rttr::type::get<ISourceProvider>();
        auto source_providers = source_provider_type.get_derived_classes();
        for (auto& pt : source_providers) {
            auto provider_var = pt.create();
            auto provider = provider_var.get_value<ISourceProvider*>();
            _source_providers[provider->type_name()] = provider;
            spdlog::info("注册数据源: [class_name={0}, type_name={1}]", pt.get_name(), provider->type_name());
        }
    }

    {
        // 注册 sinks
        auto sink_provider_type = rttr::type::get<ISinkProvider>();
        auto sink_providers = sink_provider_type.get_derived_classes();
        for (auto& pt : sink_providers) {
            auto provider_var = pt.create();
            auto provider = provider_var.get_value<ISinkProvider*>();
            _sink_providers[provider->type_name()] = provider;
            spdlog::info("注册数据接收器: [class_name={0}, type_name={1}]", pt.get_name(), provider->type_name());
        }
    }

    {
        // 注册 filters
        auto filter_provider_type = rttr::type::get<IFilterProvider>();
        auto filter_providers = filter_provider_type.get_derived_classes();
        for (auto& pt : filter_providers) {
            auto provider_var = pt.create();
            auto provider = provider_var.get_value<IFilterProvider*>();
            _filter_providers[provider->type_name()] = provider;
            spdlog::info("注册数据过滤器: [class_name={0}, type_name={1}]", pt.get_name(), provider->type_name());
        }
    }

    auto config = nlohmann::json::object();

    // 这里注册测试用的
    auto sp0 = Engine::_source_providers["source.dummy.periodic"];
    _sources.push_back(sp0->create(config, this));

    auto sp1 = Engine::_sink_providers["sink.logged"];
    _sinks.push_back(sp1->create(config));

    // 注册个管道
    IDataFlowNode* from = dynamic_cast<IDataFlowNode*>(_sources[0]);
    IDataFlowNode* to = dynamic_cast<IDataFlowNode*>(_sinks[0]);
    auto edge0 = new ForwardPipe(config, from, to);
    _pipes.push_back(edge0);
}

void Engine::emit(Msg* msg) {
    // 处理消息
    // 这里只是概念验证原型
    // 消息来源调用此函数将消息存入队列，然后引擎 worker 线程取出消息进入流水线处理

    spdlog::info("处理消息了呢");
    try {
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

    for (auto& i : _sources) {
        spdlog::info("正在启动来源线程");
        i->start();
    }

    // 引擎主线程
    auto thread = std::jthread([this](std::stop_token stoken) {
        while (!stoken.stop_requested()) {
            // 执行线程的工作
            std::this_thread::sleep_for(std::chrono::seconds(5));
            std::cout << "Worker thread is running..." << std::endl;
        }
    });
    thread.join();
}

void Engine::do_dfs(IDataFlowNode* current, MsgRoutingPath& path, Msg* msg) {

   // 将当前节点添加到路径中
    path.push_back(current);

    // 找到以当前节点为起点的所有边
    for (auto pipe : _pipes) {
        if (pipe->from() == current && pipe->is_match(msg)) {
            // 检查目标节点是否已经在路径中，以避免循环
            bool isVisited = false;
            for (auto dest_node : path) {
                if (dest_node == pipe->to()) {
                    isVisited = true;
                    break;
                }
            }

            auto target_sink_node = dynamic_cast<ISinkNode*>(pipe->to());
            if (target_sink_node != nullptr) { // 到达了收集器
                target_sink_node->receive(msg);
            } else { // 其他只可能是过滤器节点
                if (!isVisited) {
                    // 递归调用DFS来继续探索路径
                    auto target_filter_node = dynamic_cast<IFilter*>(pipe->to());
                    if(target_filter_node == nullptr) {
                    }
                    target_filter_node->filter(msg);
                    this->do_dfs(pipe->to(), path, msg);
                }
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