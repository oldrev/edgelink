#pragma once

namespace edgelink::flows {

using PropertyValue = std::variant<                     //
    std::string,                                        //
    double,                                             //
    JsonValue,                                          //
    std::chrono::time_point<std::chrono::system_clock>, //
    std::vector<uint8_t>,                               //
    bool                                                //
    >;

std::optional<JsonValue> evaluate_property_value(const JsonValue& value, const std::string_view type, const INode* node,
                                                 const std::shared_ptr<Msg>& msg);

}; // namespace edgelink::flows
