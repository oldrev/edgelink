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
    _sources.push_back(sp0->create(config));

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
}

}; // namespace edgelink