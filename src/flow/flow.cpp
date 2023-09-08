#include "edgelink/edgelink.hpp"
#include "edgelink/flow/dependency-sorter.hpp"
#include "edgelink/flow/details/flow.hpp"

using namespace edgelink;
using namespace boost;
namespace this_coro = boost::asio::this_coro;

using CloneMsgStaticVector = boost::container::static_vector<std::shared_ptr<edgelink::Msg>, 32>;

namespace edgelink::flow::details {

Flow::Flow(const boost::json::object& json_config, IEngine* engine)
    : _engine(engine), _id(json_config.at("id").as_string()), _label(json_config.at("label").as_string()),
      _disabled(edgelink::json::value_or(json_config, "disabled", true)), _nodes() {
    //
}

Flow::~Flow() {
    //
}

Awaitable<void> Flow::emit_async(const std::string_view source_node_id, std::shared_ptr<Msg> msg) {
    //
    auto source = this->get_node(source_node_id);
    auto output_ports = source->output_ports();
    for (size_t i = 0; i < output_ports.size(); i++) {
        co_await this->relay_async(source_node_id, msg, i, true);
        // 根据出度把消息复制
    }
}

Awaitable<void> Flow::start_async() {
    //
    _stop_source = std::make_unique<std::stop_source>();

    for (auto const& node : _nodes) {
        spdlog::debug("正在启动流程节点：[id={0}, type={1}]", node->id(), node->descriptor()->type_name());
        co_await node->start_async();
        spdlog::debug("流程节点已启动");
    }
}

Awaitable<void> Flow::stop_async() {
    // 给出线程池停止信号
    spdlog::debug("开始请求流程 '{0}' 停止...", this->id());
    _stop_source->request_stop();

    for (auto it = _nodes.rbegin(); it != _nodes.rend(); ++it) {
        auto ref = std::reference_wrapper<IFlowNode>(**it); // 使用 std::reference_wrapper
        spdlog::info("正在停止流程节点：[id={0}, type={1}]", ref.get().id(), ref.get().descriptor()->type_name());
        co_await ref.get().stop_async();
        spdlog::info("流程节点已停止");
    }

    spdlog::debug("流程 '{0}' 已停止", this->id());
    co_return;
}

Awaitable<void> Flow::relay_async(const std::string_view source_node_id, std::shared_ptr<Msg> orig_msg, size_t port,
                                  bool clone) const {
    auto source = this->get_node(source_node_id);
    // 根据出度把消息复制
    auto output_ports = source->output_ports();

    auto output_port = output_ports.at(port);
    for (auto j = 0; j < output_port.wires().size(); j++) {
        auto endpoint = output_port.wires().at(j);
        auto msg = clone && j > 0 ? std::make_shared<Msg>(*orig_msg) : orig_msg;

        // 线程池中处理流程
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

}; // namespace edgelink::flow::details