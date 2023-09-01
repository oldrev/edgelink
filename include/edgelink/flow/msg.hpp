#pragma once

namespace edgelink {

/*
/// @brief 消息值
using MsgValue = rva::variant<          //
    std::nullptr_t,                     // json null
    bool,                               // json boolean
    std::decimal::decimal64,            // json number
    std::string,                        // json string
    std::map<std::string, rva::self_t>, // json object, type is std::map<std::string, json_value>
    std::vector<rva::self_t>>;          // json array, type is std::vector<json_value>

using MsgObjectValue = std::map<std::string, MsgValue>;
*/

using MsgPayload = nlohmann ::json;

struct SourceNode;

/// @brief 消息结构
class Msg {
  public:
    const uint64_t id;
    const SourceNode* source;
    MsgPayload payload;

    Msg* clone() {
        auto new_msg = new Msg{
            .id = this->id,
            .source = this->source,
            .payload = this->payload,
        };
        return new_msg;
    }
};

}; // namespace edgelink