#include "edgelink/edgelink.hpp"
#include "edgelink/flow/dependency-sorter.hpp"

using namespace edgelink;
using namespace boost;
namespace this_coro = boost::asio::this_coro;

using CloneMsgStaticVector = boost::container::static_vector<std::shared_ptr<edgelink::Msg>, 32>;

namespace edgelink::flow::details {

Flow::Flow(const std::string& id) : _id(id), _nodes() {}

Flow::~Flow() {
    //
    spdlog::info("数据流关闭中...");
}

Awaitable<void> Flow::emit_async(uint32_t source_node_id, std::shared_ptr<Msg> msg) {
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
    spdlog::info("开始启动数据流引擎");
    _stop_source = std::make_unique<std::stop_source>();
    spdlog::info("数据流引擎已启动");

    for (auto const& node : _nodes) {
        spdlog::info("正在启动数据流节点：{0}", node->descriptor()->type_name());
        co_await node->start_async();
        spdlog::info("数据流节点 '{0}' 已启动", node->descriptor()->type_name());
    }
    spdlog::info("全部节点启动完毕");
}

Awaitable<void> Flow::stop_async() {
    // 给出线程池停止信号
    spdlog::info("开始请求数据流引擎线程池停止...");
    _stop_source->request_stop();

    spdlog::info("数据流引擎线程池已停止");
    co_return;
}

Awaitable<void> Flow::relay_async(uint32_t source_node_id, std::shared_ptr<Msg> orig_msg, size_t port,
                                  bool clone) const {
    auto source = this->get_node(source_node_id);
    // 根据出度把消息复制
    auto output_ports = source->output_ports();

    auto output_port = output_ports.at(port);
    for (auto j = 0; j < output_port.wires().size(); j++) {
        auto endpoint = output_port.wires().at(j);
        auto msg = clone && j > 0 ? std::make_shared<Msg>(*orig_msg) : orig_msg;

        // 线程池中处理数据流
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

}; // namespace edgelink