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

struct FlowNode;

class Msg final {
  public:
    Msg(uint32_t id, uint32_t birth_place_id)
        : _data(
              std::move(nlohmann::json::object({{"id", id}, {"birthPlaceID", birth_place_id}, {"payload", nullptr}}))) {
    }

    Msg(const Msg& other) : _data(other._data) {}

    Msg(const Msg&& other) : _data(std::move(other._data)) {}

    explicit Msg(const nlohmann::json& data) : _data(data) {}

    explicit Msg(const nlohmann::json&& data) : _data(std::move(data)) {}

    inline nlohmann::json& data() { return _data; }

    inline uint32_t id() const {
        uint32_t id = _data.at("id");
        return id;
    }

    inline uint32_t birth_place_id() const {
        uint32_t bid = _data.at("birthPlaceID");
        return bid;
    }

  private:
    nlohmann::json _data;
};

using MsgPtr = std::shared_ptr<Msg>;

}; // namespace edgelink