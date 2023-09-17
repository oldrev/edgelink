#include "edgelink/edgelink.hpp"

namespace edgelink {

std::shared_ptr<Msg> Msg::clone() const {
    //? 是否要重新生成消息 ID?
    auto new_json = JsonObject(_data);
    return std::make_shared<Msg>(std::move(new_json));
}

boost::json::value const& Msg::get_navigation_property_value(const std::string_view red_prop) const {
    auto jpath = Msg::convert_red_property_to_json_path(red_prop);
    boost::json::value const& jv = _data;
    return jv.at_pointer(std::string_view(jpath));
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