#include "../pch.hpp"

#include <croncpp/croncpp.h>

#include "edgelink/edgelink.hpp"

using namespace std;
using namespace std::decimal;

namespace edgelink {

class InjectSource : public AbstractSource {
  public:
    InjectSource(const ::nlohmann::json& config, const INodeDescriptor* desc, IMsgRouter* router)
        : AbstractSource(desc, router), _counter(0) {
        const std::string cron_expression = config["@cron"];
        _cron = ::cron::make_cron(cron_expression);
        // TODO 这里设置参数
    }

  protected:
    void process(std::stop_token& stoken) override {

        std::time_t now = std::time(0);
        std::time_t next = ::cron::cron_next(_cron, now);
        auto sleep_time = (next - now);

        std::this_thread::sleep_for(sleep_time * 1000ms);

        auto msg = new Msg{
            .id = this->router()->generate_msg_id(),
            .source = this,
            .payload = MsgPayload(),
        };

        _counter++;
        msg->payload["count"] = double(_counter);

        this->router()->emit(msg);
        spdlog::info("InjectSource > 数据已注入：[msg.id={0}]", msg->id);
    }

  private:
    int _counter;
    cron::cronexpr _cron;
};

RTTR_REGISTRATION {
    rttr::registration::class_<NodeProvider<InjectSource, "source.inject", NodeKind::SOURCE>>(
        "edgelink::LoggedSinkProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink