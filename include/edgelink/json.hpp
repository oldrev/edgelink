#pragma once

namespace edgelink {

inline const std::string_view value_or(const boost::json::object& json_obj, const std::string_view key,
                                       const std::string_view& default_value) {
    if (auto val = json_obj.if_contains(key)) {
        return std::string_view(val->as_string());
    }
    return default_value;
}

inline bool value_or(const boost::json::object& json_obj, const std::string_view key, bool default_value) {
    if (auto val = json_obj.if_contains(key)) {
        return val->as_bool();
    }
    return default_value;
}

}; // namespace edgelink