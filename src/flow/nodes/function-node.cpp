#include "edgelink/edgelink.hpp"

using namespace edgelink;

class EvalEnv final {
  public:
    EvalEnv() {}

    EvalEnv(FlowNode* node, const std::string&& msg_json_text)
        : _node(node), _msg_json_text(std::move(msg_json_text)) {}

    const std::string& msg_json_text() const { return _msg_json_text; }

    uint32_t generate_msg_id() { return _node->flow()->generate_msg_id(); }
    uint32_t node_id() const { return _node->id(); }

    template <class Inspector> static void inspect(Inspector& i) {
        i.construct(&std::make_shared<EvalEnv>);
        i.property("msgJsonText", &EvalEnv::msg_json_text);
        i.property("nodeID", &EvalEnv::node_id);
        i.method("generateMsgID", &EvalEnv::generate_msg_id);
    }

    static std::shared_ptr<EvalEnv> create(FlowNode* node, std::shared_ptr<Msg> msg) {
        auto ptr = std::make_shared<EvalEnv>(node, std::move(msg->data().dump()));
        return ptr;
    }

  private:
    FlowNode* _node;
    const std::string _msg_json_text;
};

DUK_CPP_DEF_CLASS_NAME(EvalEnv);

class FunctionNode : public FlowNode {

  public:
    FunctionNode(uint32_t id, const ::nlohmann::json& config, const INodeDescriptor* desc,
                 const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : FlowNode(id, desc, std::move(output_ports), flow) {
        _func = config.at("func");
    }

    Awaitable<void> start_async() override { co_return; }

    Awaitable<void> stop_async() override { co_return; }

    Awaitable<void> receive_async(std::shared_ptr<Msg> msg) override {
        duk::Context ctx;
        ctx.registerClass<EvalEnv>();

        auto eval_env = EvalEnv::create(this, msg);
        ctx.addGlobal("evalEnv", eval_env);

        auto js_code = fmt::format(R"(
            function jsonDeepClone(v) {{ return JSON.parse(JSON.stringify(v)); }}

            function cloneMsg(v) {{ 
                var newMsg = jsonDeepClone(v); 
                newMsg.id = evalEnv.generateMsgID(); 
                return newMsg; 
            }}

            var msg = JSON.parse(evalEnv.msgJsonText); 

            var __func_node_proc = function() {{
                {0}; 
            }};

            JSON.stringify(__func_node_proc());
            )",
                                   _func);

        std::string result_json;
        ctx.evalString(result_json, js_code.c_str());
        auto js_result = nlohmann::json::parse(result_json);

        if (js_result.type() == nlohmann::json::value_t::array) { // 多个端口消息的情况
            int port = 0;
            for (auto msg_json : js_result) {
                // 直接分发消息，只有是对象的才分发
                if (msg_json.type() == nlohmann::json::value_t::object) {
                    auto evaled_msg = std::make_shared<Msg>(std::move(msg_json));
                    co_await this->flow()->relay_async(this->id(), evaled_msg, port, true);
                }
                port++;
            }
        } else if (js_result.type() == nlohmann::json::value_t::array) { // 单个端口消息的情况
            auto evaled_msg = std::make_shared<Msg>(std::move(result_json));
            co_await this->flow()->relay_async(this->id(), evaled_msg, 0, true);
        } else { // 其他类型不支持
            spdlog::error("不支持的消息格式：{0}", result_json);
        }
    }

  private:
    std::string _func;
};

RTTR_REGISTRATION {
    rttr::registration::class_<NodeProvider<FunctionNode, "function", NodeKind::FILTER>>(
        "edgelink::FunctionNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};
