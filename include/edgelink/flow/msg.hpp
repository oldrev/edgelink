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

  public:
    Msg(uint64_t id, const SourceNode* source) : id(id), source(source), payload() {}

    /// @brief 拷贝构造函数
    /// @param msg
    explicit Msg(const Msg& msg) : id(msg.id), source(msg.source), payload(msg.payload) {}

    /// @brief 指定 ID 的拷贝构造函数
    /// @param msg 
    /// @param id 
    Msg(const Msg& msg, uint64_t id) : id(id), source(msg.source), payload(msg.payload) {}

};

}; // namespace edgelink