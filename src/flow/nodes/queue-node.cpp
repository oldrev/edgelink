#include "../../pch.hpp"
#include "edgelink/edgelink.hpp"

using namespace std;

namespace edgelink {

class QueueNode : public FilterNode {
  public:
    QueueNode(uint32_t id, const ::nlohmann::json& config, const INodeDescriptor* desc,
              const std::vector<OutputPort>& output_ports, IMsgRouter* router)
        : FilterNode(id, desc, output_ports, router), _queue(config.value("capacity", 100)) {
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
        spdlog::info("QueueNode > 收到了消息：[msg.id={0}]", msg->id);
        _queue.wait_push_back(msg);
    }

    bool is_running() const { return _thread.joinable(); }

  private:
    boost::sync_bounded_queue<std::shared_ptr<Msg>> _queue;
    std::jthread _thread; // 一个数据源一个线程
};

RTTR_REGISTRATION {
    rttr::registration::class_<NodeProvider<QueueNode, "queue", NodeKind::FILTER>>("edgelink::QueueNodeProvider")
        .constructor()(rttr::policy::ctor::as_std_shared_ptr);
};

}; // namespace edgelink