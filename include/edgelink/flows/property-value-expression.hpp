#pragma once

namespace edgelink::flows {

std::optional<JsonValue> evaluate_property_value(const JsonValue& value, const std::string_view type, const INode* node,
                                                 const std::shared_ptr<Msg>& msg);

}; // namespace edgelink::flows
