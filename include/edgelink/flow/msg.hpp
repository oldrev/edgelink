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

struct FlowNode;

/// @brief 消息结构
class Msg {
  public:
    const uint64_t id;
    const FlowNode* birth_place;
    std::optional<std::chrono::system_clock::time_point> birth_time;
    MsgPayload payload;

  public:
    Msg(uint64_t id, const FlowNode* birth_place) : id(id), birth_place(birth_place), payload() {}

    /// @brief 拷贝构造函数
    /// @param msg
    explicit Msg(const Msg& msg) : id(msg.id), birth_place(msg.birth_place), payload(msg.payload) {}

    /// @brief 指定 ID 的拷贝构造函数
    /// @param msg
    /// @param id
    Msg(const Msg& msg, uint64_t id) : id(id), birth_place(msg.birth_place), payload(msg.payload) {}
};

}; // namespace edgelink