#pragma once

namespace edgelink {

struct FlowNode;
class Msg;

using MsgID = uint32_t;
using MsgPtr = std::shared_ptr<Msg>;

class EDGELINK_EXPORT Msg final : private boost::noncopyable {
  public:
    Msg() : Msg(Msg::generate_msg_id()) {}

    Msg(MsgID id) : _data(std::move(VariantObject({{"_msgid", id}, {"payload", nullptr}}))) {}

    Msg(Msg&& other) : _data(std::move(other._data)) {}

    explicit Msg(const Variant& data) : _data(data) {}

    explicit Msg(Variant&& data) : _data(std::move(data)) {}

    Variant& data() & {
        auto const& self = *this;
        return const_cast<Variant&>(self._data);
    }

    Variant&& data() && { return std::move(this->data()); }

    Variant const& data() const& { return _data; }

    inline MsgID id() const {
        MsgID id = static_cast<MsgID>(_data.at("_msgid").get<int64_t>());
        return id;
    }

    void set_id(MsgID new_id);

    MsgPtr clone() const;

    inline const JsonString to_json_string() const { return JsonString(std::move(_data.json_dump())); }

    inline const std::string to_string() const { return _data.json_dump(); }

    Variant const& at_propex(const std::string_view propex) const&;

    Variant& at_propex(const std::string_view propex) & {
        auto const& self = *this;
        return const_cast<Variant&>(self.at_propex(propex));
    }
    Variant&& at_propex(const std::string_view propex) && { return std::move(this->at_propex(propex)); }

    Variant const& at(const std::string_view prop) const& { return _data.at(prop); }

    Variant& at(const std::string_view prop) & {
        auto const& self = *this;
        return const_cast<Variant&>(self.at(prop));
    }

    Variant&& at(const std::string_view prop) && { return std::move(this->at(prop)); }

    void insert_or_assign(const std::string_view prop_expr, Variant&& value) {
        this->data().as_object().insert_or_assign(prop_expr, std::move(value));
    }

    static MsgID generate_msg_id();

  private:
    Variant _data;
};


}; // namespace edgelink
