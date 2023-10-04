#pragma once

#include <edgelink/flows/variant.hpp>

namespace edgelink {

struct INode;
class Msg;

};


namespace edgelink::propex {

const size_t PROPERTY_SEGMENT_MAX = 16;

enum class PropertySegmentKindIndex : size_t {
    IDENTIFIER = 0,
    INT_INDEX,
};

using PropertySegment = std::variant<std::string_view, size_t>;

using PropertySegments = boost::container::static_vector<PropertySegment, PROPERTY_SEGMENT_MAX>;

EDGELINK_EXPORT bool try_parse(const std::string_view input, PropertySegments& result);

EDGELINK_EXPORT const PropertySegments parse(const std::string_view input);

EDGELINK_EXPORT Variant evaluate_property_value(const Variant& value, const std::string_view type, const INode& node,
                                                const Msg& msg);

}; // namespace edgelink::flows::propex
