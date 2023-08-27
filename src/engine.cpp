#include "pch.hpp"

#include "edgelink/edgelink.hpp"

using namespace std;

namespace edgelink {

Engine::Engine(const nlohmann::json& json_config)
    : _config{.queue_capacity = 100}, _msg_queue(boost::sync_bounded_queue<Msg*>(100)) {

    // 注册 sinks
    auto sink_provider_type = rttr::type::get<ISinkProvider>();
    auto sink_providers = sink_provider_type.get_derived_classes();
    for (auto& pt : sink_providers) {
        auto provider_var = pt.create();
        // auto provider = rttr::rttr_cast<ISinkProvider*>(&provider_var);
        if(provider_var.convert(sink_provider_type)) {
            auto x = rttr::rttr_cast<ISinkProvider>(provider_var);
        }
        //spdlog::info("发现数据接收器: [class_name={0}, type_name={1}]", pt.get_name(), provider->type_name());
        // auto x = pt.invoke("create", provider_var, {json_config});
        // auto sink = rttr::variant_cast<ISinkNode*>(&x);
        // const ISinkProvider* x = provider.get_wrapped_value<>();
        // spdlog::info("发现数据接收器: [class_name={0}, type_name={1}]", pt.get_name(), provider->type_name());
    }

    auto config = nlohmann::json::object();

    // 这里注册测试用的
    // auto sp0 = Engine::s_source_providers["source.dummy.periodic"];
    // sp0->create(config);
    //_sources.push_back(sp0->create(config));
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

    for (auto& i : _sinks) {
        spdlog::info("正在启动接收器线程：[type={0}]", typeid(i).name());
        i->start();
    }

    /*
    for (auto& i : _filters) {
        spdlog::info("正在启动过滤器：[type={0}]", i.first);
        i.second->start();
    }
    */

    for (auto& i : _sources) {
        // spdlog::info("正在启动来源线程：[type={0}]", i.first);
        i->start();
    }
}

}; // namespace edgelink