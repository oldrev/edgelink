#include "edgelink/edgelink.hpp"
#include "edgelink/variant.hpp"
#include "edgelink/propex.hpp"

namespace json = boost::json;

namespace edgelink {

static const JsonValue variant_to_json(const Variant& obj) {
    JsonValue jvalue;
    Variant::Kind kind = obj.kind();
    switch (kind) {
    case Variant::Kind::NULLPTR: {
        jvalue = JsonValue(nullptr);
    } break;

    case Variant::Kind::BOOL: {
        jvalue = JsonValue(obj.get<bool>());
    } break;

    case Variant::Kind::INT64: {
        jvalue = JsonValue(obj.get<int64_t>());
    } break;

    case Variant::Kind::DOUBLE: {
        jvalue = JsonValue(obj.get<double>());
    } break;

    case Variant::Kind::STRING: {
        jvalue = JsonValue(obj.get<std::string>());
    } break;

    case Variant::Kind::BUFFER: {
        TODO("暂时不支持");
    } break;

    /*
    case Variant::Kind::DATE: {
        TODO("暂时不支持");
    } break;
    */

    case Variant::Kind::OBJECT: {
        VariantObject const& map = obj.get<VariantObject>();
        JsonObject jo;
        for (const auto& [key, child] : map) {
            auto jv = variant_to_json(child);
            jo.emplace(std::move(key), std::move(jv));
        }
        jvalue = jo;
    } break;

    case Variant::Kind::ARRAY: {
        VariantArray const& array = obj.get<VariantArray>();
        JsonArray ja;
        for (const auto& child : array) {
            auto jv = variant_to_json(child);
            ja.emplace_back(std::move(jv));
        }
        jvalue = ja;
    } break;

    default:
        throw std::runtime_error("Wrong Variant kind");
    }

    return jvalue;
}

Variant const& Variant::at_propex(const std::string_view propex) const& {
    auto prop_segs = propex::parse(propex);
    const Variant* presult = this;
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

JsonValue Variant::to_json() const {
    //
    return variant_to_json(_data);
}

}; // namespace edgelink
