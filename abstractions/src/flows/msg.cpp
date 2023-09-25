#include "edgelink/edgelink.hpp"

namespace edgelink {

MsgPtr Msg::clone() const {
    //? 是否要重新生成消息 ID?
    // TODO FIXME
    auto new_data = Variant::from_json(_data.to_json());
    return std::make_shared<Msg>(std::move(new_data));
}

void Msg::set_id(MsgID new_id) {
    if (!_data.is_object()) {
        _data = JsonObject{{"_msgid", new_id}};
    } else {
        this->insert_or_assign("_msgid", new_id);
    }
}

Variant const& Msg::at_propex(const std::string_view propex) const& {
    auto prop_segs = propex::parse(propex);
    const Variant* presult = &this->_data;
    for (auto const& ps : prop_segs) {
        if (ps.index() == static_cast<size_t>(propex::PropertySegmentKindIndex::IDENTIFIER)) {
            std::string key(std::get<std::string_view>(ps));
            presult = &(presult->at(key));
        } else {
            auto index = std::get<size_t>(ps);
            presult = &(presult->at(index));
        }
    }
    return *presult;
}

MsgID Msg::generate_msg_id() {
    static std::atomic<uint32_t> msg_id_counter(0); // 初始化计数器为0
    if (msg_id_counter.load() >= 0xFFFFFFF0) {
        msg_id_counter.store(0);
        return msg_id_counter.fetch_add(1);
    } else {
        return msg_id_counter.fetch_add(1);
    }
}

}; // namespace edgelink