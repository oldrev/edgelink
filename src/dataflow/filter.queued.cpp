#include "../pch.hpp"
#include "edgelink/edgelink.hpp"

using namespace std;

namespace edgelink {

class QueuedFilter : public AbstractFilter {
  public:
    QueuedFilter(const ::nlohmann::json& config, const INodeDescriptor* desc, IMsgRouter* router)
        : AbstractFilter(desc, router), _queue(100) {}

    void start() override {}

    void stop() override {}

    shared_ptr<Msg> filter(shared_ptr<Msg> msg) override {
        _queue.wait_push_back(msg);
        spdlog::info("QueuedFilter: 收到了消息：[msg.id={0}]", msg->id);

        shared_ptr<Msg> ret_msg = nullptr;
        _queue.wait_pull_front(ret_msg);
        return ret_msg;
    }

  private:
    boost::sync_bounded_queue<std::shared_ptr<Msg>> _queue;
};

RTTR_REGISTRATION {
    rttr::registration::class_<NodeProvider<QueuedFilter, "filter.queued", NodeKind::FILTER>>(
        "edgelink::QueuedFilterProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink