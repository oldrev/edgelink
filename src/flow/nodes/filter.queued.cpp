#include "../../pch.hpp"
#include "edgelink/edgelink.hpp"

using namespace std;

namespace edgelink {

class QueuedFilter : public FilterNode {
  public:
    QueuedFilter(const ::nlohmann::json& config, const INodeDescriptor* desc, IMsgRouter* router)
        : FilterNode(desc, router), _queue(config.value("capacity", 100)) {
        //
    }

    void start() override {
        if (!_thread.joinable()) {
            _thread = std::jthread([this](std::stop_token stoken) {
                // 线程函数
                while (!stoken.stop_requested()) {
                    shared_ptr<Msg> msg = nullptr;
                    _queue.wait_pull_front(msg);
                    this->router()->relay(this, msg);
                }
            });
        }
    }

    void stop() override {
        if (_thread.joinable()) {
            _thread.request_stop();
            _thread.join();
        }
    }

    void receive(const std::shared_ptr<Msg>& msg) override {
        spdlog::info("QueuedFilter > 收到了消息：[msg.id={0}]", msg->id);
        _queue.wait_push_back(msg);
    }

    bool is_running() const { return _thread.joinable(); }

  private:
    boost::sync_bounded_queue<std::shared_ptr<Msg>> _queue;
    std::jthread _thread; // 一个数据源一个线程
};

RTTR_REGISTRATION {
    rttr::registration::class_<NodeProvider<QueuedFilter, "filter.queued", NodeKind::FILTER>>(
        "edgelink::QueuedFilterProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink