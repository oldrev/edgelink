#include "edgelink/edgelink.hpp"
#include <bustache/model.hpp>
#include <bustache/render/string.hpp>
#include <bustache/format.hpp>

using namespace edgelink;

namespace bustache {
template <> struct impl_model<boost::json::object> {
    static constexpr model kind = model::object;
};

template <> struct impl_compatible<boost::json::value> {
    static value_ptr get_value_ptr(boost::json::value const& self) {
        // Use non-const version for reference.
        auto& ref = const_cast<boost::json::value&>(self);
        switch (ref.kind()) {
        case boost::json::kind::null:
            break;
        case boost::json::kind::bool_:
            return value_ptr(&ref.get_bool());
        case boost::json::kind::int64:
            return value_ptr(&ref.get_int64());
        case boost::json::kind::uint64:
            return value_ptr(&ref.get_uint64());
        case boost::json::kind::double_:
            return value_ptr(&ref.get_double());
        case boost::json::kind::string:
            return value_ptr(&ref.get_string());
        case boost::json::kind::array:
            return value_ptr(&ref.get_array());
        case boost::json::kind::object:
            return value_ptr(&ref.get_object());
        }
        return value_ptr();
    }
};

template <> struct impl_object<boost::json::object> {
    static void get(boost::json::object const& self, std::string const& key, value_handler visit) {
        auto const it = self.find(key);
        visit(it == self.end() ? nullptr : &it->value());
    }
};

}; // namespace bustache

struct BustacheContext : std::unordered_map<std::string, bustache::format> {
    using unordered_map::unordered_map;

    bustache::format const* operator()(std::string const& key) const {
        auto it = find(key);
        return it == end() ? nullptr : &it->second;
    }
};

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

        boost::json::value wrapped_msg = boost::json::object({
            {_field, msg->data().at(_field)},
        });
        const std::string json_text = bustache::to_string(_format(wrapped_msg).escape(bustache::escape_html));
        auto parsed_msg_field_value = boost::json::parse(json_text);

        msg->data()[_field] = parsed_msg_field_value;

        co_await this->flow()->relay_async(this->id(), msg, 0, true);

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
