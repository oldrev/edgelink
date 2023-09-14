
#include <quickjs/quickjs.h>
#include <quickjspp.hpp>

#include "edgelink/edgelink.hpp"

using namespace edgelink;

/*
    {
        "id": "588889e4e95dd46c",
        "type": "function",
        "z": "7c226c13f2e3b224",
        "name": "价值",
        "func": "msg.payload = msg.payload % 100;\nreturn msg;",
        "outputs": 2,
        "noerr": 0,
        "initialize": "// 部署节点后，此处添加的代码将运行一次。 \nvar xxxx = 0;",
        "finalize": "// 节点正在停止或重新部署时，将运行此处添加的代码。 \nvar xxxxx = 0;",
        "libs": [],
        "x": 290,
        "y": 320,
        "wires": [
            [
                "b1c267019d45655a",
                "3596fa8fefd657e4"
            ],
            []
        ]
    }
*/

class EvalEnv final {
  public:
    EvalEnv() {}

    EvalEnv(FlowNode* node, const std::string&& msg_json_text)
        : _node(node), _msg_json_text(std::move(msg_json_text)) {}

    const std::string& msg_json_text() const { return _msg_json_text; }

    MsgID generate_msg_id() { return Msg::generate_msg_id(); }
    const std::string node_id() const { return std::string(_node->id()); }

    static std::shared_ptr<EvalEnv> create(FlowNode* node, std::shared_ptr<Msg> msg) {
        auto ptr = std::make_shared<EvalEnv>(node, std::move(boost::json::serialize(msg->data())));
        return ptr;
    }

  private:
    FlowNode* _node;
    const std::string _msg_json_text;
};

constexpr char JS_PRELUDE[] = R"(
function jsonDeepClone(v) { return JSON.parse(JSON.stringify(v)); }

function cloneMsg(v) {
    var newMsg = jsonDeepClone(v); 
    newMsg.id = evalEnv.generateMsgID();
    return newMsg;
}

)";

constexpr char JS_CODE_TEMPLATE[] = R"(
    var __func_node_user_func = function() {{ 
		var msg = JSON.parse(evalEnv.msgJsonText); 
        var user_func = function() {{
        /* 用户代码开始 */
        {0}; 
        /* 用户代码结束 */
        }};
        return JSON.stringify(user_func());
    }};
)";

struct ModuleEntry {
    const std::string module;
    const std::string var;
};

class FunctionNode : public FlowNode {

  public:
    FunctionNode(const std::string_view id, const boost::json::object& config, const INodeDescriptor* desc, IFlow* flow)
        : FlowNode(id, desc, flow, config), _outputs(config.at("outputs").to_number<size_t>()),
          _func(config.at("func").as_string()), _initialize(config.at("initialize").as_string()),
          _finalize(config.at("finalize").as_string()), _runtime(), _context(_runtime) {

        for (auto const& module_json : config.at("libs").as_array()) {
            auto const& entry = module_json.as_object();
            auto me = ModuleEntry{
                std::string(entry.at("module").as_string()),
                std::string(entry.at("var").as_string()),
            };
            _modules.emplace_back(std::move(me));
        }

        auto& m = _context.addModule("EdgeLink");
        // m.function<&println>("println");
        m.class_<EvalEnv>("EvalEnv")
            .fun<&EvalEnv::generate_msg_id>("generateMsgID")
            .property<&EvalEnv::msg_json_text>("msgJsonText")
            .property<&EvalEnv::node_id>("nodeID");

        // 加载前置代码

        // 加载 EdgeLink 模块
        _context.eval(R"xxx(
            import * as el from 'EdgeLink';
            globalThis.el = el;
        )xxx",
                      "<import>", JS_EVAL_TYPE_MODULE);
        auto prelude_value = _context.eval(JS_PRELUDE);
        this->logger()->info("PRELUDE RESULT: {0}", prelude_value.as<const std::string>());
    }

    Awaitable<void> async_start() override { co_return; }

    Awaitable<void> async_stop() override { co_return; }

    Awaitable<void> receive_async(std::shared_ptr<Msg> msg) override {

        auto eval_env = EvalEnv::create(this, msg);
        _context.global()["evalEnv"] = eval_env.get();

        auto js_code = fmt::format(JS_CODE_TEMPLATE, _func);

        try {

            _context.eval(js_code);
            auto user_func_callback = (std::function<std::string()>)_context.eval("__func_node_user_func");
            const std::string result_json = user_func_callback();

            // 后续处理执行成果
            auto js_result = boost::json::parse(result_json);

            if (js_result.kind() == boost::json::kind::array) { // 多个端口消息的情况
                auto array = js_result.as_array();
                if (array.size() > this->output_ports().size()) {
                    auto error_msg = "JS 脚本输出错误的端口数";
                    this->logger()->error(error_msg);
                    throw std::out_of_range(error_msg);
                }
                std::vector<std::shared_ptr<Msg>> msgs;

                for (auto& msg_json_value : array) {
                    // 直接分发消息，只有是对象的才分发
                    if (msg_json_value.kind() == boost::json::kind::object) {
                        auto msg_json = msg_json_value.as_object();
                        auto evaled_msg = std::make_shared<Msg>(msg_json);
                        msgs.emplace_back(std::move(evaled_msg));
                    }
                }
                co_await this->async_send_to_many_port(std::forward<std::vector<std::shared_ptr<Msg>>>(msgs));
            } else if (js_result.kind() == boost::json::kind::object) { // 单个端口消息的情况
                auto object_result = js_result.as_object();
                auto evaled_msg = std::make_shared<Msg>(std::move(object_result));
                co_await this->async_send_to_one_port(std::move(evaled_msg));
            } else { // 其他类型不支持
                this->logger()->error("不支持的消息格式：{0}", result_json);
            }
        } catch (qjs::exception) {
            auto exc = _context.getException();
            this->logger()->error("QuickJS 错误：{0}", static_cast<const std::string>(exc));
            // TODO 这里报告错误给 flow
            co_return;
        }
        catch (std::exception& ex) {
            this->logger()->error("错误：{0}", ex.what());
            throw;
        }
        co_return;
    }

  private:
    const size_t _outputs;
    const std::string _func;
    const std::string _initialize;
    const std::string _finalize;
    unsigned int _noerr;
    std::vector<ModuleEntry> _modules;
    qjs::Runtime _runtime;
    qjs::Context _context;
};

RTTR_REGISTRATION {
    rttr::registration::class_<FlowNodeProvider<FunctionNode, "function", NodeKind::PIPE>>(
        "edgelink::FunctionNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};
