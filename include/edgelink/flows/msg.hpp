#pragma once

namespace edgelink {

using MsgID = uint32_t;

/*
using Bytes = std::vector<uint8_t>;

enum class MsgValueKindIndex {
    NULLPTR = 0,
    BOOL = 0,
    DOUBLE = 0,
    STRING = 0,
    BYTES = 0,
    OBJECT = 0,
    ARRAY = 0,
};

/// @brief 消息值
using MsgValue = rva::variant<          ///
    std::nullptr_t,                     ///< null
    bool,                               ///< boolean
    double,                             ///< number
    std::string,                        ///< string
    Bytes,                              ///< bytes buffer
    std::map<std::string, rva::self_t>, ///< object, type is std::map<std::string, json_value>
    std::vector<rva::self_t>>;          ///< array, type is std::vector<json_value>
*/

struct FlowNode;

using JsonPointerExpression = boost::static_string<256>;

class Msg final : private boost::noncopyable {
  public:
    Msg() : Msg(Msg::generate_msg_id()) {}

    Msg(MsgID id) : _data(std::move(boost::json::object({{"_msgid", id}, {"payload", nullptr}}))) {}

    Msg(Msg&& other) : _data(std::move(other._data)) {}

    explicit Msg(boost::json::object const& data) : _data(data) {}

    explicit Msg(boost::json::object&& data) : _data(std::move(data)) {}

    inline boost::json::object& data() { return _data; }

    inline MsgID id() const {
        MsgID id = _data.at("_msgid").to_number<MsgID>();
        return id;
    }

    inline void set_id(MsgID new_id) { this->set_property_value("_msgid", new_id); }

    std::shared_ptr<Msg> clone() const;

    inline const boost::json::string to_json_string() const {
        return boost::json::string(std::move(boost::json::serialize(_data)));
    }
    inline const std::string to_string() const { return boost::json::serialize(_data); }

    boost::json::value const& get_navigation_property_value(const std::string_view red_prop) const;

    const boost::json::value& get_property_value(const std::string_view prop_expr) const { return _data.at(prop_expr); }

    template <typename TValue> void set_property_value(const std::string_view prop_expr, const TValue& value) {
        auto it = _data.find(prop_expr);
        if (it != _data.end()) {
            it->value() = value;
        } else {
            _data.emplace(prop_expr, value);
        }
    }

    static MsgID generate_msg_id();

    static JsonPointerExpression convert_red_property_to_json_path(const std::string_view prop) {
        // TODO 检查参数长度
        auto path_to_replace = JsonPointerExpression(prop.begin(), prop.end());
        for (auto it = path_to_replace.begin(); it != path_to_replace.end(); ++it) {
            if (*it == '.') {
                *it = '/';
            }
        }
        auto ret = JsonPointerExpression("/");
        ret.append(path_to_replace);
        return ret;
    }

  private:
    boost::json::object _data;
};

using MsgPtr = std::shared_ptr<Msg>;

}; // namespace edgelink
