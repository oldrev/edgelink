#pragma once

namespace edgelink {

enum class VariantKind : size_t {
    NULLPTR,
    BOOL,
    F64,
    I64,
    STRING,
    BUFFER,
    OBJECT,
    ARRAY,
};

using Variant =
    rva::variant<std::nullptr_t,                     // NULL
                 bool,                               // boolean
                 double,                             // F64
                 int64_t,                            // int64_t
                 std::string,                        // json string
                 std::vector<uint8_t>,               // buffer
                 std::map<std::string, rva::self_t>, // json object, type is std::map<std::string, json_value>
                 std::vector<rva::self_t>>;          // json array, type is std::vector<json_value>

inline VariantKind kind(const Variant& value) { return static_cast<VariantKind>(value.index()); }

}; // namespace edgelink