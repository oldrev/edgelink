#pragma once

namespace edgelink {

using MsgID = uint32_t;

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
    Msg(MsgID id, FlowNodeID birth_place_id)
        : _data(
              std::move(nlohmann::json::object({{"id", id}, {"birthPlaceID", birth_place_id}, {"payload", nullptr}}))) {
    }

    Msg(const Msg& other) : _data(other._data) {}

    Msg(const Msg&& other) : _data(std::move(other._data)) {}

    explicit Msg(const nlohmann::json& data) : _data(data) {}

    explicit Msg(const nlohmann::json&& data) : _data(std::move(data)) {}

    inline nlohmann::json& data() { return _data; }

    inline MsgID id() const {
        MsgID id = _data.at("id");
        return id;
    }

    inline FlowNodeID birth_place_id() const {
        FlowNodeID bid = _data.at("birthPlaceID");
        return bid;
    }

    static MsgID generate_msg_id() {
        static std::atomic<uint32_t> msg_id_counter(0); // 初始化计数器为0
        if (msg_id_counter.load() >= 0xFFFFFFF0) {
            msg_id_counter.store(0);
            return msg_id_counter.fetch_add(1);
        } else {
            return msg_id_counter.fetch_add(1);
        }
    }

  private:
    nlohmann::json _data;
};

using MsgPtr = std::shared_ptr<Msg>;

}; // namespace edgelink