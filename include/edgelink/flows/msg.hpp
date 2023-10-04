#pragma once

#include "variant.hpp"

namespace edgelink {

using MsgID = uint32_t;
struct FlowNode;

class EDGELINK_EXPORT Msg final : private boost::noncopyable {
  public:
    Msg() : Msg(Msg::generate_msg_id()) {}

    Msg(MsgID id) : _data(std::move(Variant({{"_msgid", id}, {"payload", nullptr}}))) {}

    Msg(Msg&& other) : _data(std::move(other._data)) {}

    explicit Msg(Variant const& data) : _data(data) {}

    explicit Msg(Variant&& data) : _data(std::move(data)) {}

    inline Variant& data() { return _data.as_object(); }

    inline Variant const& data() const { return _data.as_object(); }

    inline MsgID id() const {
        MsgID id = rva::get<std::map<Variant>>(_data).at("_msgid").to_number<MsgID>();
        return id;
    }

    void set_id(MsgID new_id);

    std::shared_ptr<Msg> clone() const;

    //inline const JsonString to_json_string() const { return JsonString(std::move(boost::json::serialize(_data))); }

    inline const std::string to_string() const { return boost::json::serialize(_data); }

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
        this->data().insert_or_assign(prop_expr, std::move(value));
    }

    static MsgID generate_msg_id();

  private:
    Variant _data;
};

using MsgPtr = std::shared_ptr<Msg>;

}; // namespace edgelink
