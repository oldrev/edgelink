#include "edgelink/edgelink.hpp"
#include <bustache/format.hpp>
#include <bustache/adapted/boost_json.hpp>

using namespace edgelink;

class TemplateNode : public FlowNode {

  public:
    TemplateNode(FlowNodeID id, const boost::json::object& config, const INodeDescriptor* desc,
                 const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : FlowNode(id, desc, std::move(output_ports), flow, config),
          _field(config.at("field").as_string()),          // .field 属性
          _field_type(config.at("fieldType").as_string()), // .fieldType 属性
          _format(config.at("template").as_string())

    {
        //
    }

    Awaitable<void> start_async() override { co_return; }

    Awaitable<void> stop_async() override { co_return; }

    Awaitable<void> receive_async(std::shared_ptr<Msg> msg) override {
        //
        // bustache::context context;
        std::stringstream ss;
        ss << _format(msg->data());

        auto parsed = boost::json::parse(ss).as_object();

        auto new_msg = std::make_shared<Msg>(parsed);

        co_await this->flow()->relay_async(this->id(), new_msg, 0, true);

        co_return;
    }

  private:
    const bustache::format _format;
    const std::string _field;
    const std::string _field_type;
};

RTTR_REGISTRATION {
    rttr::registration::class_<NodeProvider<TemplateNode, "template", NodeKind::FILTER>>(
        "edgelink::TemplateNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};
