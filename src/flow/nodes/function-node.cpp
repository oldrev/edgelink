#include "edgelink/edgelink.hpp"

#include "edgelink/scripting/duktape.hpp"

using namespace edgelink;
using namespace edgelink::scripting;

class EvalEnv final {
  public:
    EvalEnv() {}

    EvalEnv(FlowNode* node, const std::string&& msg_json_text)
        : _node(node), _msg_json_text(std::move(msg_json_text)) {}

    const std::string& msg_json_text() const { return _msg_json_text; }

    MsgID generate_msg_id() { return Msg::generate_msg_id(); }
    FlowNodeID node_id() const { return _node->id(); }

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

static const char* JS_PRELUDE = R"(
function jsonDeepClone(v) { return JSON.parse(JSON.stringify(v)); }

function cloneMsg(v) {
    var newMsg = jsonDeepClone(v); 
    newMsg.id = evalEnv.generateMsgID(); 
    return newMsg; 
}

)";

class FunctionNode : public FlowNode {

  public:
    FunctionNode(FlowNodeID id, const boost::json::object& config, const INodeDescriptor* desc,
                 const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : FlowNode(id, desc, std::move(output_ports), flow) {
        _func = config.at("func").as_string();

        _ctx.registerClass<EvalEnv>();

        // 加载前置代码
        _ctx.evalStringNoRes(JS_PRELUDE);
    }

    Awaitable<void> start_async() override { co_return; }

    Awaitable<void> stop_async() override { co_return; }

    Awaitable<void> receive_async(std::shared_ptr<Msg> msg) override {

        // 保存 Duktape 状态到全局 stash
        duk_push_global_stash(_ctx);
        duk_dup(_ctx, 0); // 复制当前的 Duktape 状态到 stash
        duk_put_prop_string(_ctx, -2, "__savedContext");
        duk_pop(_ctx); // 弹出全局 stash

        auto eval_env = EvalEnv::create(this, msg);
        _ctx.addGlobal("evalEnv", eval_env);

        auto js_code = fmt::format(R"(
            var msg = JSON.parse(evalEnv.msgJsonText); 
            var __func_node_proc = function() {{ {0}; }};
            JSON.stringify(__func_node_proc());
            )",
                                   _func);

        boost::json::string result_json;
        _ctx.evalString(result_json, js_code.c_str());

        // 后续处理执行成果
        auto js_result = boost::json::parse(result_json);

        if (js_result.kind() == boost::json::kind::array) { // 多个端口消息的情况
            int port = 0;
            auto array = js_result.as_array();
            for (auto& msg_json_value : array) {
                // 直接分发消息，只有是对象的才分发
                if (msg_json_value.kind() == boost::json::kind::object) {
                    auto msg_json = msg_json_value.as_object();
                    auto evaled_msg = std::make_shared<Msg>(std::move(msg_json));
                    co_await this->flow()->relay_async(this->id(), evaled_msg, port, true);
                }
                port++;
            }
        } else if (js_result.kind() == boost::json::kind::object) { // 单个端口消息的情况
            auto object_result = js_result.as_object();
            auto evaled_msg = std::make_shared<Msg>(std::move(object_result));
            co_await this->flow()->relay_async(this->id(), evaled_msg, 0, true);
        } else { // 其他类型不支持
            spdlog::error("不支持的消息格式：{0}", result_json);
        }

        // 恢复之前保存的 Duktape 状态
        duk_push_global_stash(_ctx);
        duk_get_prop_string(_ctx, -1, "__savedContext"); // 获取保存的状态
        duk_dup(_ctx, -1);                               // 复制 stash 中的状态到当前堆栈
        duk_pop_2(_ctx);                                 // 弹出全局 stash 和恢复的状态
    }

  private:
    std::string _func;
    duk::Context _ctx;
};

RTTR_REGISTRATION {
    rttr::registration::class_<NodeProvider<FunctionNode, "function", NodeKind::FILTER>>(
        "edgelink::FunctionNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};
