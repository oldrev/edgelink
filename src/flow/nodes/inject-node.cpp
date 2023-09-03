
#include "edgelink/edgelink.hpp"

using namespace std;
namespace this_coro = boost::asio::this_coro;

namespace edgelink {

class InjectNode : public SourceNode {
  public:
    const char* DEFAULT_CRON = "*/5 * * * * ?"; // 默认值是每隔两秒执行一次
  public:
    InjectNode(uint32_t id, const ::nlohmann::json& config, const INodeDescriptor* desc,
               const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : SourceNode(id, desc, move(output_ports), flow) {
        const std::string cron_expression = config.value("cron", DEFAULT_CRON);
        _cron = ::cron::make_cron(cron_expression);
        // TODO 这里设置参数
    }

  protected:
    Awaitable<void> process_async(std::stop_token& stoken) override {

        auto executor = co_await this_coro::executor;

        std::time_t now = std::time(0);
        std::time_t next = ::cron::cron_next(_cron, now);
        auto sleep_time = (next - now);
        spdlog::info("InjectNode > time={0} next={1} now={2}", sleep_time, next, now);

        auto msg_id = this->flow()->generate_msg_id();
        auto msg = make_shared<Msg>(msg_id, this->id());

        if (spdlog::get_level() >= spdlog::level::info) {
            spdlog::info("InjectNode > 数据已注入：[msg={0}]", msg->data().dump());
        }

        co_await this->flow()->emit_async(this->id(), msg);

        boost::asio::steady_timer timer(executor, std::chrono::milliseconds(1000));
        co_await timer.async_wait(boost::asio::use_awaitable);
    }

  private:
    cron::cronexpr _cron;
};

RTTR_REGISTRATION {
    rttr::registration::class_<NodeProvider<InjectNode, "inject", NodeKind::SOURCE>>("edgelink::InjectNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink