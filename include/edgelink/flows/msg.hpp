#pragma once

namespace edgelink {

using MsgID = uint32_t;

struct FlowNode;

using JsonPointerExpression = boost::static_string<256>;

class EDGELINK_EXPORT Msg final : private boost::noncopyable {
  public:
    Msg() : Msg(Msg::generate_msg_id()) {}

    Msg(MsgID id) : _data(std::move(JsonObject({{"_msgid", id}, {"payload", nullptr}}))) {}

    Msg(Msg&& other) : _data(std::move(other._data)) {}

    explicit Msg(JsonObject const& data) : _data(data) {}

    explicit Msg(JsonObject&& data) : _data(std::move(data)) {}

    inline JsonObject& data() { return _data.as_object(); }

    inline JsonObject const& data() const { return _data.as_object(); }

    inline MsgID id() const {
        MsgID id = _data.at("_msgid").to_number<MsgID>();
        return id;
    }

    void set_id(MsgID new_id);

    std::shared_ptr<Msg> clone() const;

    inline const JsonString to_json_string() const { return JsonString(std::move(boost::json::serialize(_data))); }
    inline const std::string to_string() const { return boost::json::serialize(_data); }

    JsonValue const& at_propex(const std::string_view propex) const&;

    JsonValue& at_propex(const std::string_view propex) & {
        auto const& self = *this;
        return const_cast<JsonValue&>(self.at_propex(propex));
    }
    JsonValue&& at_propex(const std::string_view propex) && { return std::move(this->at_propex(propex)); }

    JsonValue const& at(const std::string_view prop) const& { return _data.at(prop); }

    JsonValue& at(const std::string_view prop) & {
        auto const& self = *this;
        return const_cast<JsonValue&>(self.at(prop));
    }

    JsonValue&& at(const std::string_view prop) && { return std::move(this->at(prop)); }

    void insert_or_assign(const std::string_view prop_expr, JsonValue&& value) {
        this->data().insert_or_assign(prop_expr, std::move(value));
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
    JsonValue _data;
};

using MsgPtr = std::shared_ptr<Msg>;

}; // namespace edgelink
