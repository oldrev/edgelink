#include "edgelink/edgelink.hpp"
#include "edgelink/flows/property-expression.hpp"

namespace edgelink::flows {

namespace json = boost::json;

// Assuming you have a getMessageProperty function
// and other required functions declared and defined.

std::optional<JsonValue> evaluate_property_value(const JsonValue& value, const std::string_view type, const INode* node,
                                                 const std::shared_ptr<Msg>& msg) {

    auto result = std::optional<JsonValue>();

    if (type == "str") {
        result = value;
    } else if (type == "num") {
        if (value.is_string()) {
            try {
                result = boost::lexical_cast<double>(value.as_string());

            } catch (const std::exception&) {
                throw std::runtime_error("Invalid number format");
                return result;
            }
        } else if (value.is_number()) {
            result = value;
        } else {
            throw std::runtime_error("Invalid number format");
        }
    } else if (type == "json") {
        try {
            if (value.is_string()) {
                result = json::parse(value.as_string());
            } else {
                result = value;
            }
        } catch (const std::exception&) {
            throw std::runtime_error("Invalid JSON format");
        }
    } else if (type == "re") {
        /*
        try {
            result = std::regex(value.as_string().c_str());
        } catch (const std::exception&) {
            if (callback) {
                callback("Invalid regular expression");
            } else {
                throw std::runtime_error("Invalid regular expression");
            }
            return result;
        }
        */
        TODO("不支持正则");
    } else if (type == "date") {
        // Your date conversion logic here
        auto now = std::chrono::system_clock::now();
        time_t time = std::chrono::system_clock::to_time_t(now);
        result = double(time);
    } else if (type == "bin") {
        /*
        try {
            JsonValue data = json::parse(value.as_string());
            if (data.is_array() || data.is_string()) {
                const auto& bin_str = data.as_string();
                result = JsonArray(std::vector<uint8_t>(bin_str.begin(), bin_str.end()));
            } else {
                throw std::runtime_error("Invalid buffer data");
            }
        } catch (const std::exception&) {
            throw std::runtime_error("Invalid JSON format");
        }
        */
        TODO("暂时没实现");
    } else if (type == "msg" && msg) {
        result = JsonValue(msg->get_navigation_property_value(value.as_string()));
    } else if ((type == "flow" || type == "global") && node != nullptr) {
        /*
        ContextKey contextKey = parseContextStore(value.as_string().c_str());
        if (std::regex_search(contextKey.key, std::regex("\\[msg"))) {
            // The key has a nested msg reference to evaluate first
            contextKey.key = normalisePropertyExpression(contextKey.key, msg, true);
        }
        result = node->context()[type]->get(contextKey.key, contextKey.store, callback);
        */
        TODO("暂时不知支持");
    } else if (type == "bool") {
        result = json::parse(value.as_string());
    } else if (type == "jsonata") {
        TODO("暂时不知支持 jsonata");
    } else if (type == "env") {
        TODO("暂时不知支持 env");
    }

    return result;
}

}; // namespace edgelink::flows