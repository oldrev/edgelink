#pragma once

#include "json.hpp"

namespace edgelink {

enum class DynamicObjectKindIndex : unsigned char {
    NULLPTR, ///< null
    BOOL,    ///< boolean
    DOUBLE,  ///< number
    STRING,  ///< string
    JSON,    //
    BUFFER,  ///< bytes buffer
    DATE,    //
    OBJECT,  ///< object, type is std::map<std::string, json_value>
    ARRAY    ///< array, type is std::vector<json_value>
};

using DynamicObject = rva::variant<                     ///
    std::nullptr_t,                                     ///< null
    bool,                                               ///< boolean
    double,                                             ///< number
    std::string,                                        ///< string
    JsonValue,                                          ///< JSON Value
    std::vector<uint8_t>,                               ///< bytes buffer
    std::chrono::time_point<std::chrono::system_clock>, ///< Date
    std::map<std::string, rva::self_t>,                 ///< object, type is std::map<std::string, json_value>
    std::vector<rva::self_t>>;                          ///< array, type is std::vector<json_value>

}; // namespace edgelink