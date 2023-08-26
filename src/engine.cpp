#include "pch.h"

#include "edgelink/transport/modbus.hpp"

using namespace std;

namespace edgelink {

std::vector<const ISourceProvider*> Engine::s_source_providers;
std::vector<const IPipeProvider*> Engine::s_filter_providers;
std::vector<const ISinkProvider*> Engine::s_sink_providers;

void Engine::register_source(const ISourceProvider* provider) {
    Engine::s_source_providers.push_back(provider);
    std::cout << "已注册数据源：" << provider->type_name() << std::endl;
}

void Engine::register_sink(const ISinkProvider* provider) {
    Engine::s_sink_providers.push_back(provider);
    std::cout << "已注册接收器：" << provider->type_name() << std::endl;
}

void Engine::register_filter(const IPipeProvider* provider) {
    Engine::s_filter_providers.push_back(provider);
    std::cout << "已注册过滤器：" << provider->type_name() << std::endl;
}

Engine::Engine() {

    // 这里注册测试用的
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

    std::cout << "All threads have completed." << std::endl;

    for (auto& i : _sinks) {
        spdlog::info("正在启动接收器线程：[type={0}]", i.first);
        i.second->start();
    }

    for (auto& i : _filters) {
        spdlog::info("正在启动过滤器：[type={0}]", i.first);
        i.second->start();
    }

    for (auto& i : _sources) {
        spdlog::info("正在启动来源线程：[type={0}]", i.first);
        i.second->start();
    }
}

}; // namespace edgelink