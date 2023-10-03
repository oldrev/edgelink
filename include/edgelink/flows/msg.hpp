#pragma once

namespace edgelink {

using MsgID = uint32_t;

enum class MsgValueKind : unsigned char {
    NULLPTR,
    DOUBLE,
    INT64,
    BOOL,
    STRING,
    BUFFER,
};

using MsgValue = boost::variant< //
    std::nullptr_t,              //
    double,                      //
    int64_t,                     //
    bool,                        //
    std::string,                 //
    std::vector<uint8_t>         //
    >;

inline MsgValueKind kind(const MsgValue& value) { return static_cast<MsgValueKind>(value.which()); }

struct FlowNode;

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

  private:
    JsonValue _data;
};

using MsgPtr = std::shared_ptr<Msg>;

}; // namespace edgelink
