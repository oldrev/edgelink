
#include <croncpp/croncpp.h>

#include "edgelink/edgelink.hpp"

namespace this_coro = boost::asio::this_coro;

namespace edgelink {

class InjectNode : public SourceNode {
  public:
    const char* DEFAULT_CRON = "*/5 * * * * ?"; // 默认值是每隔两秒执行一次
  public:
    InjectNode(FlowNodeID id, const boost::json::object& config, const INodeDescriptor* desc,
               const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : SourceNode(id, desc, move(output_ports), flow, config),
          _cron(::cron::make_cron(config.contains("crontab") ? config.at("crontab").as_string() : DEFAULT_CRON)) {
        //
    }

  protected:
    Awaitable<void> process_async(std::stop_token& stoken) override {

        auto executor = co_await this_coro::executor;

        auto msg_id = Msg::generate_msg_id();
        auto msg = std::make_shared<Msg>(msg_id, this->id());

        if (spdlog::get_level() >= spdlog::level::info) {
            spdlog::info("InjectNode > 数据已注入：[msg={0}]", boost::json::serialize(msg->data()));
        }

        co_await this->flow()->emit_async(this->id(), msg);

        std::time_t now = std::time(0);
        std::time_t next = ::cron::cron_next(_cron, now);
        auto sleep_time = (next - now);

        boost::asio::steady_timer timer(executor, std::chrono::seconds(sleep_time));
        co_await timer.async_wait(boost::asio::use_awaitable);
        co_return;
    }

  private:
    cron::cronexpr _cron;
};

RTTR_REGISTRATION {
    rttr::registration::class_<NodeProvider<InjectNode, "inject", NodeKind::SOURCE>>("edgelink::InjectNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink