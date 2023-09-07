#include "edgelink/edgelink.hpp"
#include "edgelink/flow/dependency-sorter.hpp"

using namespace boost;
namespace this_coro = boost::asio::this_coro;

using CloneMsgStaticVector = boost::container::static_vector<std::shared_ptr<edgelink::Msg>, 32>;

namespace edgelink {

Engine::Engine(const EdgeLinkConfig& el_config, const IFlowFactory& flow_factory)
    : _flows_json_path(el_config.flows_json_path), _flow_factory(flow_factory) {

    // std::vector<std::unique_ptr<IFlow>> create_flows(const boost::json::array& flows_config);
}

Engine::~Engine() {
    //
    // TODO 同步关闭
    spdlog::info("流程引擎清理中...");

    asio::io_context io_context(1);
    asio::co_spawn(io_context, this->stop_async(), asio::detached);
    io_context.run();

    spdlog::info("流程引擎已关闭");
}

Awaitable<void> Engine::start_async() {

    // TODO 检查是否在运行

    spdlog::info("流程引擎 > 开始加载流配置：'{0}'", _flows_json_path);

    _flows.clear();

    std::ifstream flows_file(_flows_json_path);
    auto flows_config =
        boost::json::parse(flows_file, {}, {.allow_comments = true, .allow_trailing_commas = true}).as_array();

    if (flows_config.size() == 0) {
        throw BadFlowConfigException("There are no node in the configuration file of the flows!");
    }

    auto global_nodes = _flow_factory.create_global_nodes(flows_config);
    for (auto& gn : global_nodes) {
        _global_nodes.emplace_back(std::move(gn));
    }

    auto flows = _flow_factory.create_flows(flows_config);
    for (auto& flow : flows) {
        _flows.emplace_back(std::move(flow));
    }

    //
    spdlog::info("开始启动流程引擎");
    _stop_source = std::make_unique<std::stop_source>();

    for (auto& flow : _flows) {
        spdlog::info("正在启动流程：{0}", flow->id());
        co_await flow->start_async();
    }
    spdlog::info("全部流程启动完毕");
    spdlog::info("流程引擎已启动");
}

Awaitable<void> Engine::stop_async() {
    // 给出线程池停止信号
    spdlog::info("开始请求流程引擎停止...");
    _stop_source->request_stop();

    for (auto it = _flows.rbegin(); it != _flows.rend(); ++it) {
        auto ref = std::reference_wrapper<IFlow>(**it); // 使用 std::reference_wrapper
        co_await ref.get().stop_async();
    }

    spdlog::info("流程引擎已停止");
    co_return;
}

}; // namespace edgelink