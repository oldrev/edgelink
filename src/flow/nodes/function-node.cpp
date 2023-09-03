#include "edgelink/edgelink.hpp"

using namespace std;
using namespace edgelink;

class EvalEnv final {
  public:
    explicit EvalEnv(const string& msg_json_text) : _msg_json_text(msg_json_text) {
        //
    }

    const string& msg_json_text() const { return _msg_json_text; }

    template <class Inspector> static void inspect(Inspector& i) {
        i.construct(&std::make_shared<EvalEnv, const string&>);
        i.property("msg_json_text", &EvalEnv::msg_json_text);
    }

  private:
    string _msg_json_text;
};

DUK_CPP_DEF_CLASS_NAME(EvalEnv);

class FunctionNode : public FlowNode {

  public:
    FunctionNode(uint32_t id, const ::nlohmann::json& config, const INodeDescriptor* desc,
                 const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : FlowNode(id, desc, move(output_ports), flow) {
        _func = config.at("func");
    }

    void start() override {}

    void stop() override {}

    void receive(shared_ptr<Msg> msg) override {
        duk::Context ctx;
        ctx.registerClass<EvalEnv>();

        auto eval_env = EvalEnv(msg->data().dump());
        ctx.addGlobal("evalEnv", eval_env);

        auto js_code = fmt::format("var msg = JSON.parse(evalEnv.msg_json_text); {0}; JSON.stringify(msg)", _func);

        string result_json;
        ctx.evalString(result_json, js_code.c_str());

        auto evaled_msg = make_shared<Msg>(nlohmann::json::parse(result_json));

        // 直接分发消息
        this->flow()->relay(this->id(), evaled_msg, 0, true);
    }

  private:
    std::string _func;
};

RTTR_REGISTRATION {
    rttr::registration::class_<NodeProvider<FunctionNode, "function", NodeKind::FILTER>>(
        "edgelink::FunctionNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};
