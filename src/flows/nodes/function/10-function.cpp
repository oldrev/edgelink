
#include <duktape.h>
#include <duktape-cpp/DuktapeCpp.h>

#include "edgelink/edgelink.hpp"
#include "edgelink/scripting/duktape.hpp"

using namespace edgelink;
using namespace edgelink::scripting;

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

    template <class Inspector> static void inspect(Inspector& i) {
        i.construct(&std::make_shared<EvalEnv>);
        i.property("msgJsonText", &EvalEnv::msg_json_text);
        i.property("nodeID", &EvalEnv::node_id);
        i.method("generateMsgID", &EvalEnv::generate_msg_id);
    }

    static std::shared_ptr<EvalEnv> create(FlowNode* node, std::shared_ptr<Msg> msg) {
        auto ptr = std::make_shared<EvalEnv>(node, std::move(boost::json::serialize(msg->data())));
        return ptr;
    }

  private:
    FlowNode* _node;
    const std::string _msg_json_text;
};

DUK_CPP_DEF_CLASS_NAME(EvalEnv);

constexpr char JS_PRELUDE[] = R"(
function jsonDeepClone(v) { return JSON.parse(JSON.stringify(v)); }

function cloneMsg(v) {
    var newMsg = jsonDeepClone(v); 
    newMsg.id = evalEnv.generateMsgID(); 
    return newMsg; 
}

)";

constexpr char JS_CODE_TEMPLATE[] = R"(
    var msg = JSON.parse(evalEnv.msgJsonText); 
    var __func_node_proc = function() {{ {0}; }};
    JSON.stringify(__func_node_proc());
)";

struct ModuleEntry {
    const std::string module;
    const std::string var;
};

class FunctionNode : public FlowNode {

  public:
    FunctionNode(const std::string_view id, const boost::json::object& config, const INodeDescriptor* desc,
                 const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : FlowNode(id, desc, std::move(output_ports), flow, config), _func(config.at("func").as_string()),
          _outputs(config.at("outputs").to_number<size_t>()), _initialize(config.at("initialize").as_string()),
          _finalize(config.at("finalize").as_string()) {

        for (auto const& module_json : config.at("libs").as_array()) {
            auto const& entry = module_json.as_object();
            auto me = ModuleEntry{
                std::string(entry.at("module").as_string()),
                std::string(entry.at("var").as_string()),
            };
            _modules.emplace_back(std::move(me));
        }

        _ctx.registerClass<EvalEnv>();

        // 加载前置代码
        _ctx.evalStringNoRes(JS_PRELUDE);
    }

    Awaitable<void> async_start() override { co_return; }

    Awaitable<void> async_stop() override { co_return; }

    Awaitable<void> receive_async(std::shared_ptr<Msg> msg) override {

        edgelink::scripting::DuktapeStashingGuard stash_guard(_ctx);

        auto eval_env = EvalEnv::create(this, msg);
        _ctx.addGlobal("evalEnv", eval_env);

        auto js_code = fmt::format(JS_CODE_TEMPLATE, _func);

        boost::json::string result_json;
        _ctx.evalString(result_json, js_code.c_str());

        // 后续处理执行成果
        auto js_result = boost::json::parse(result_json);

        if (js_result.kind() == boost::json::kind::array) { // 多个端口消息的情况
            int port = 0;
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
        co_return;
    }

  private:
    const size_t _outputs;
    const std::string _func;
    const std::string _initialize;
    const std::string _finalize;
    unsigned int _noerr;
    std::vector<ModuleEntry> _modules;
    duk::Context _ctx;
};

RTTR_REGISTRATION {
    rttr::registration::class_<FlowNodeProvider<FunctionNode, "function", NodeKind::FILTER>>(
        "edgelink::FunctionNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};
