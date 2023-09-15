
#include <croncpp.h>

#include "edgelink/edgelink.hpp"

namespace this_coro = boost::asio::this_coro;
namespace asio = boost::asio;

using namespace edgelink;

/*
   {
    "id": "00c80d77c0c5a9de",
    "type": "inject",
    "z": "73e0fcd142fc5256",
    "name": "",
    "props": [
        {
            "p": "payload"
        },
        {
            "p": "topic",
            "vt": "str"
        },
        {
            "p": "123",
            "v": "123",
            "vt": "flow"
        }
    ],
    "repeat": "",
    "crontab": "",
    "once": false,
    "onceDelay": 0.1,
    "topic": "",
    "payload": "",
    "payloadType": "date",
    "x": 270,
    "y": 80,
    "wires": [
        []
    ]
}
*/

struct PropertyEntry {
    std::string p;
    std::optional<std::string> vt;
    std::optional<boost::json::value> v;

    PropertyEntry(const std::string_view p, const std::string_view vt, const boost::json::value& v)
        : p(p), vt(vt), v(v) {
        //
    }

    explicit PropertyEntry(const boost::json::object& obj) {
        this->p = std::string(obj.at("p").as_string());

        if (auto jvt = obj.if_contains("vt")) {
            this->vt = std::string(jvt->as_string());
        }

        if (auto jv = obj.if_contains("v")) {
            this->v = *jv;
        }
    }
};

class InjectNode : public SourceNode {
  public:
    InjectNode(const std::string_view id, const boost::json::object& config, const INodeDescriptor* desc, IFlow* flow)
        : SourceNode(id, desc, flow, config), _once(config.at("once").as_bool()) {

        if (auto props_value = config.if_contains("props")) {
            for (auto const& prop : props_value->as_array()) {
                auto const& entry = prop.as_object();
                PropertyEntry pe(entry);
                _props.emplace_back(std::move(pe));
            }
        }

        /* Handle legacy */
        if (config.if_contains("props")) {
            for (size_t i = 0, l = _props.size(); i < l; i++) {
                if (_props.at(i).p == "payload" && !_props[i].v) {
                    _props[i].v = config.at("payload");
                    _props[i].vt = std::string(config.at("payloadType").as_string());
                } else if (_props.at(i).p == "topic" && _props.at(i).vt == "str" && !_props.at(i).v) {
                    _props[i].v = config.at("topic");
                }
            }
        } else {
            _props.emplace_back(
                PropertyEntry("payload", std::string(config.at("payloadType").as_string()), config.at("payload")));

            if (auto topic_value = config.if_contains("topic")) {
                _props.emplace_back(PropertyEntry("topic", "str", *topic_value));
            }
        }

        if (auto repeat_value = config.if_contains("repeat")) {
            const std::string_view repeat_str = repeat_value->as_string();
            if (!repeat_str.empty()) {
                _repeat = (uint64_t)std::floor(boost::lexical_cast<double>(repeat_str) * 1000);
            }
        }

        if (auto crontab_value = config.if_contains("crontab")) {
            const std::string_view crontab_str = crontab_value->as_string();
            if (!crontab_str.empty()) {
                _cron = ::cron::make_cron(crontab_str);
            }
        }

        if (auto once_delay_value = config.if_contains("onceDelay")) {
            auto once_delay =
                *once_delay_value == "" ? 0.1 : once_delay_value->to_number<double>();
            if (once_delay != NAN && once_delay > 0) {
                _once_delay = (uint64_t)std::floor(once_delay * 1000);
            }
        }
    }

  protected:
    Awaitable<void> on_async_run() override {
        auto executor = co_await this_coro::executor;

        if (_once) {
            co_await this->async_once_task(executor);
        }

        // 进入循环执行
        if (_repeat && *_repeat > 0) {
            co_await this->async_repeat_task(executor);
        } else if (_cron) {
            co_await this->async_cron_task(executor);
        } else {
            throw std::logic_error("Bad repeat condition");
        }

        co_return;
    }

  private:
    std::shared_ptr<Msg> create_msg() {
        auto msg = std::make_shared<Msg>();

        auto currentTime = std::chrono::system_clock::now();

        // 将当前时间点转换为毫秒
        auto sinceEpoch = currentTime.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(sinceEpoch);

        // 获取毫秒时间戳
        int64_t milliseconds = millis.count();

        for (auto const& prop : _props) {
            if (prop.p == "payload") {
                msg->set_property_value(prop.p, milliseconds);
            } else {
                if(prop.v) {
                    msg->set_property_value(prop.p, *prop.v);
                }
            }
        }

        return msg;
    }

    Awaitable<void> async_once_task(asio::any_io_executor executor) {
        auto msg = this->create_msg();
        co_await this->async_send_to_one_port(std::move(msg));
        co_return;
    }

    Awaitable<void> async_cron_task(asio::any_io_executor executor) {
        while (true) { // TODO  改成等待 stop_token
            std::time_t now = std::time(0);
            std::time_t next = ::cron::cron_next(*_cron, now);
            auto sleep_time = (next - now);

            asio::steady_timer timer(executor, std::chrono::milliseconds(sleep_time));
            co_await timer.async_wait(asio::use_awaitable);

            auto msg = this->create_msg();
            co_await this->async_send_to_one_port(std::move(msg));
        }
        co_return;
    }

    Awaitable<void> async_repeat_task(asio::any_io_executor executor) {

        while (true) { // TODO  改成等待 stop_token
            asio::steady_timer timer(executor, std::chrono::milliseconds(*_repeat));
            co_await timer.async_wait(asio::use_awaitable);

            auto msg = this->create_msg();
            co_await this->async_send_to_one_port(std::move(msg));
        }
        co_return;
    }

  private:
    // 各个字段
    std::optional<uint64_t> _repeat;
    std::optional<cron::cronexpr> _cron;
    bool _once;
    std::optional<uint64_t> _once_delay;
    std::vector<PropertyEntry> _props;
};

RTTR_REGISTRATION {
    rttr::registration::class_<FlowNodeProvider<InjectNode, "inject", NodeKind::SOURCE>>("edgelink::InjectNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};