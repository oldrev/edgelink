#pragma once

namespace edgelink::var {

enum class VarKind : size_t {
    NULLPTR,
    BOOL,
    I64,
    F64,
    STRING,
    BUFFER,
    OBJECT,
    ARRAY,
};

using Var = rva::variant<                            //
    std::nullptr_t,                                  // NULL
    bool,                                            // boolean
    int64_t,                                         // int64_t
    double,                                          // F64
    std::string,                                     // json string
    std::vector<uint8_t>,                            // buffer
    std::map<std::string, rva::self_t, std::less<>>, // json object, type is std::map<std::string,
    std::vector<rva::self_t>>;                       // json array, type is std::vector<json_value>


template <VarKind KIND_VALUE>
struct KindToVarIndex {
    static constexpr size_t INDEX = static_cast<size_t>(KIND_VALUE);
};

using VarObject = std::variant_alternative_t<KindToVarIndex<VarKind::OBJECT>::INDEX, Var>;

using VarArray = std::variant_alternative_t<KindToVarIndex<VarKind::ARRAY>::INDEX, Var>;

inline VarKind kind(const Var& value) { return static_cast<VarKind>(value.index()); }

}; // namespace edgelink::var
