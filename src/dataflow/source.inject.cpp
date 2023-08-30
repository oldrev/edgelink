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
            .source = this,
            .payload = MsgPayload(),
        };

        _counter++;
        msg->payload["count"] = decimal64(_counter);

        this->router()->emit(msg);
        spdlog::info("InjectSource: 数据已注入");
    }

  private:
    int _counter;
    cron::cronexpr _cron;
};

struct InjectSourceProvider : public INodeProvider, public INodeDescriptor {
  public:
    InjectSourceProvider() : _type_name("source.inject") {}

    IDataFlowNode* create(const ::nlohmann::json& config, IMsgRouter* router) const override {
        return new InjectSource(config, this, router);
    }

    const INodeDescriptor* descriptor() const override { return this; }
    const std::string_view& type_name() const override { return _type_name; }
    const NodeKind kind() const override { return NodeKind::SOURCE; }

  private:
    const string_view _type_name;

    RTTR_ENABLE(INodeProvider)
};

}; // namespace edgelink

RTTR_REGISTRATION {
    rttr::registration::class_<edgelink::InjectSourceProvider>("edgelink::InjectSourceProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
}