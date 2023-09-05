#include "edgelink/edgelink.hpp"

#include <boost/mustache.hpp>

using namespace edgelink;

class TemplateNode : public FlowNode {

  public:
    TemplateNode(FlowNodeID id, const boost::json::object& config, const INodeDescriptor* desc,
                 const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : FlowNode(id, desc, std::move(output_ports), flow, config),
          _field(config.at("field").as_string()),          // .field 属性
          _field_type(config.at("fieldType").as_string()), // .fieldType 属性
          _template(config.at("template").as_string())

    {
        //
    }

    Awaitable<void> start_async() override { co_return; }

    Awaitable<void> stop_async() override { co_return; }

    Awaitable<void> receive_async(std::shared_ptr<Msg> msg) override {

        boost::json::value wrapped_msg = boost::json::object({
            {_field, boost::json::value(msg->data().at(_field))},
        });
        std::stringstream out;
        boost::mustache::render(_template, out, wrapped_msg, {});

        auto json_text = out.str();
        auto parsed_msg_field_value = boost::json::parse(json_text);

        msg->data()[_field] = parsed_msg_field_value;

        co_await this->flow()->relay_async(this->id(), msg, 0, true);

        co_return;
    }

  private:
    const std::string _template;
    const std::string _field;
    const std::string _field_type;
};

RTTR_REGISTRATION {
    rttr::registration::class_<NodeProvider<TemplateNode, "template", NodeKind::FILTER>>(
        "edgelink::TemplateNodeProvider")
        .constructor()(rttr::policy::ctor::as_raw_ptr);
};
