#include "edgelink/edgelink.hpp"
#include "edgelink/flows/dependency-sorter.hpp"
#include "flow.hpp"

using namespace edgelink;
using namespace boost;
namespace this_coro = boost::asio::this_coro;

using CloneMsgStaticVector = boost::container::static_vector<std::shared_ptr<edgelink::Msg>, 32>;

namespace edgelink::flow::details {

Flow::Flow(const boost::json::object& json_config, IEngine* engine)
    : _logger(spdlog::default_logger()->clone("Flow")), _engine(engine), _id(json_config.at("id").as_string()),
      _label(json_config.at("label").as_string()), _disabled(edgelink::json::value_or(json_config, "disabled", true)),
      _nodes() {
    //
}

Flow::~Flow() {
    //
}

Awaitable<void> Flow::start_async() {
    //
    _stop_source = std::make_unique<std::stop_source>();

    for (auto const& node : _nodes) {
        _logger->debug("正在启动流程节点：[id={0}, type={1}]", node->id(), node->descriptor()->type_name());
        co_await node->start_async();
        _logger->debug("流程节点已启动");
    }
}

Awaitable<void> Flow::stop_async() {
    // 给出线程池停止信号
    _logger->debug("开始请求流程 '{0}' 停止...", this->id());
    _stop_source->request_stop();

    for (auto it = _nodes.rbegin(); it != _nodes.rend(); ++it) {
        auto ref = std::reference_wrapper<IFlowNode>(**it); // 使用 std::reference_wrapper
        _logger->info("正在停止流程节点：[id={0}, type={1}]", ref.get().id(), ref.get().descriptor()->type_name());
        co_await ref.get().stop_async();
        _logger->info("流程节点已停止");
    }

    _logger->debug("流程 '{0}' 已停止", this->id());
    co_return;
}

Awaitable<void> Flow::async_send_many(const std::vector<Envelope>&& envelopes) {
    for (auto& e : envelopes) {
        co_await this->async_send_one(std::forward<const Envelope>(e));
    }
    co_return;
}

Awaitable<void> Flow::async_send_one(const Envelope&& e) {

    this->on_send_event()(this, e);

    if (e.source_node->descriptor()->kind() == NodeKind::SOURCE) {
        auto exec = co_await this_coro::executor;
        // 根据出度把消息复制，这里是异步非阻塞的
        boost::asio::co_spawn(exec, this->async_send_one_internal(std::move(e)), boost::asio::detached);
    } else {
        co_await this->async_send_one_internal(std::move(e));
    }
    co_return;
}

Awaitable<void> Flow::async_send_one_internal(const Envelope&& envelope) {

    this->on_pre_route_event()(this, envelope);

    auto msg = envelope.clone_message ? std::make_shared<Msg>(*envelope.msg) : envelope.msg;

    // 线程池中处理流程
    //
    bool can_deliver = false;
    switch (envelope.destination_node->descriptor()->kind()) {

    case NodeKind::FILTER:
    case NodeKind::SINK:
    case NodeKind::JUNCTION: {
        can_deliver = true;
    } break;

    default:
        auto error_msg = fmt::format("错误的节点类型 [msg_id={}, source={}, destination={}]", envelope.msg->id(),
                                     envelope.source_id, envelope.destination_id);
        _logger->error(error_msg);
        throw InvalidDataException(error_msg);
    }

    if (can_deliver) {
        this->on_pre_deliver_event()(this, envelope);
        co_await envelope.destination_node->receive_async(msg);
        this->on_post_deliver_event()(this, envelope);
    }
}

}; // namespace edgelink::flow::details