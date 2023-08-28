#include "../pch.hpp"

#include "edgelink/edgelink.hpp"

using namespace std;
using namespace std::decimal;

namespace edgelink {

class DummyPeriodicSource : public AbstractSource {
  public:
    DummyPeriodicSource(const ::nlohmann::json& config, IMsgRouter* router) : AbstractSource(router), _counter(0) {
        // TODO 这里设置参数
    }

  protected:
    void process(std::stop_token& stoken) override {
        // TODO 产生消息给 Engine
        std::this_thread::sleep_for(1000ms);
        spdlog::info("DummyPeriodicSource: 产生时间");

        auto msg = new Msg{
            .source = this,
            .payload = MsgPayload(),
        };

        _counter++;
        msg->payload["count"] = decimal64(_counter);

            this->router()->emit(msg);
    }

  private:
    int _counter;
};

struct DummyPeriodicSourceProvider : public ISourceProvider {
  public:
    DummyPeriodicSourceProvider() : _type_name("source.dummy.periodic") {}

    const std::string_view& type_name() const override { return _type_name; }
    ISourceNode* create(const ::nlohmann::json& config, IMsgRouter* router) const override {
        return new DummyPeriodicSource(config, router);
    }

  private:
    const string_view _type_name;

    RTTR_ENABLE(ISourceProvider)
};

}; // namespace edgelink

RTTR_REGISTRATION {
    rttr::registration::class_<edgelink::DummyPeriodicSourceProvider>("edgelink::DummyPeriodicSourceProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
}