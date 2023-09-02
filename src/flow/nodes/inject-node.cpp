#include "../../pch.hpp"

#include <croncpp/croncpp.h>

#include "edgelink/edgelink.hpp"

using namespace std;

namespace edgelink {

class InjectNode : public SourceNode {
  public:
    const char* DEFAULT_CRON = "*/5 * * * * ?"; // 默认值是每隔两秒执行一次
  public:
    InjectNode(uint32_t id, const ::nlohmann::json& config, const INodeDescriptor* desc,
               const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : SourceNode(id, desc, move(output_ports), flow), _counter(0) {
        const std::string cron_expression = config.value("cron", DEFAULT_CRON);
        _cron = ::cron::make_cron(cron_expression);
        // TODO 这里设置参数
    }

  protected:
    void process(std::stop_token& stoken) override {

        std::time_t now = std::time(0);
        std::time_t next = ::cron::cron_next(_cron, now);
        auto sleep_time = (next - now);

        std::this_thread::sleep_for(sleep_time * 1000ms);

        auto msg_id = this->flow()->generate_msg_id();
        auto msg = make_shared<Msg>(msg_id, this);

        _counter++;
        msg->payload["count"] = double(_counter);

        this->flow()->emit(msg);
        spdlog::info("InjectNode > 数据已注入：[msg.id={0}]", msg->id);
    }

  private:
    int _counter;
    cron::cronexpr _cron;
};

RTTR_REGISTRATION {
    rttr::registration::class_<NodeProvider<InjectNode, "inject", NodeKind::SOURCE>>("edgelink::InjectNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};

}; // namespace edgelink