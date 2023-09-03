#include "edgelink/edgelink.hpp"

using namespace std;
using namespace boost;
namespace this_coro = boost::asio::this_coro;

namespace edgelink {

/// @brief 此类暂时不可用
class QueueNode : public FilterNode {
  public:
    QueueNode(uint32_t id, const ::nlohmann::json& config, const INodeDescriptor* desc,
              const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : FilterNode(id, desc, std::move(output_ports), flow), _queue(config.value("capacity", 100)) {
        //
    }

    Awaitable<void> start_async() override {

        auto executor = co_await this_coro::executor;
        auto loop = std::bind(&QueueNode::work_loop, this);
        asio::co_spawn(executor, loop, asio::detached);

        co_return;
    }

    Awaitable<void> stop_async() override { co_return; }

    Awaitable<void> receive_async(std::shared_ptr<Msg> msg) override {
        //
        _queue.wait_push_back(msg);
        co_return;
    }

    bool is_running() const { return true; }

  private:
    Awaitable<void> work_loop() {
        auto stoken = _stop.get_token();
        while (!stoken.stop_requested()) {
            std::shared_ptr<Msg> msg;
            _queue.wait_pull_front(msg);
            co_await this->flow()->relay_async(this->id(), msg, 0, true);
        }
        co_return;
    }

  private:
    std::stop_source _stop;
    boost::sync_bounded_queue<std::shared_ptr<Msg>> _queue;
};

RTTR_REGISTRATION {
    rttr::registration::class_<NodeProvider<QueueNode, "queue", NodeKind::FILTER>>("edgelink::QueueNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink