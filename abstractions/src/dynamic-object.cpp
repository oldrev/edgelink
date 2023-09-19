
#include "edgelink/edgelink.hpp"
#include "edgelink/flows/propex.hpp"
#include "edgelink/dynamic-object.hpp"

namespace edgelink {

/*
NULLPTR, ///< null
BOOL,    ///< boolean
DOUBLE,  ///< number
STRING,  ///< string
JSON,    //
BUFFER,  ///< bytes buffer
DATE,    //
OBJECT,  ///< object, type is std::map<std::string, json_value>
ARRAY    ///< array, type is std::vector<json_value>
*/

static DynamicObjectKind get_kind(const DynamicObjectVariant& obj) {
    return static_cast<DynamicObjectKind>(obj.index());
}

static const JsonValue dynamic_object_variant_to_json(const DynamicObjectVariant& obj) {
    JsonValue value;
    DynamicObjectKind kind = get_kind(obj);
    switch (kind) {
    case DynamicObjectKind::NULLPTR: {
        value = JsonValue(nullptr);
    } break;

    case DynamicObjectKind::BOOL: {
        value = JsonValue(rva::get<bool>(obj));
    } break;

    case DynamicObjectKind::DOUBLE: {
        value = JsonValue(rva::get<double>(obj));
    } break;

    case DynamicObjectKind::STRING: {
        value = JsonValue(rva::get<std::string>(obj));
    } break;

    case DynamicObjectKind::BUFFER: {
        TODO("暂时不支持");
    } break;

    case DynamicObjectKind::DATE: {
        TODO("暂时不支持");
    } break;

    case DynamicObjectKind::OBJECT: {
        auto map = rva::get<std::map<std::string, DynamicObjectVariant>>(obj);
        JsonObject jo;
        for (const auto& [key, child] : map) {
            auto jv = dynamic_object_variant_to_json(child);
            jo.emplace(std::move(key), std::move(jv));
        }
        value = jo;
    } break;

    case DynamicObjectKind::ARRAY: {
        auto array = rva::get<std::vector<DynamicObjectVariant>>(obj);
        JsonArray ja;
        for (const auto& child : array) {
            auto jv = dynamic_object_variant_to_json(child);
            ja.emplace_back(std::move(jv));
        }
        value = ja;
    } break;

    default:
        throw std::runtime_error("Wrong DynamicObjectVariant kind");
    }

    return JsonValue();
}

const JsonValue DynamicObject::to_json() const { return dynamic_object_variant_to_json(_variant); }

const std::string DynamicObject::dump_json() const { return boost::json::serialize(this->to_json()); }

}; // namespace edgelink
