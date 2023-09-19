#pragma once

#include "json.hpp"

namespace edgelink {

enum class EDGELINK_EXPORT DynamicObjectKind : unsigned char {
    NULLPTR, ///< null
    BOOL,    ///< boolean
    DOUBLE,  ///< number
    STRING,  ///< string
    BUFFER,  ///< bytes buffer
    DATE,    //
    OBJECT,  ///< object, type is std::map<std::string, json_value>
    ARRAY    ///< array, type is std::vector<json_value>
};

using DynamicObjectVariant =
    rva::variant<std::nullptr_t,                                     ///< null
                 bool,                                               ///< boolean
                 double,                                             ///< number
                 std::string,                                        ///< string
                 std::vector<uint8_t>,                               ///< bytes buffer
                 std::chrono::time_point<std::chrono::system_clock>, ///< Date
                 std::map<std::string, rva::self_t>, ///< object, type is std::map<std::string, json_value>
                 std::vector<rva::self_t>>;          ///< array, type is std::vector<json_value>

class EDGELINK_EXPORT DynamicObject {
  public:
    DynamicObject() : _variant(nullptr) {}

    DynamicObject(const DynamicObject& other) : _variant(other._variant) {}
    DynamicObject(DynamicObject&& other) noexcept : _variant(std::move(other._variant)) {}

    DynamicObject(std::nullptr_t other) noexcept : _variant(other) { }
    DynamicObject(bool other) noexcept : _variant(other) {  }
    DynamicObject(double other) noexcept { _variant = other; }
    DynamicObject(const std::string& other) noexcept : _variant(other) {}
    DynamicObject(std::string&& other) noexcept : _variant(std::move(other)) {}
    DynamicObject(const std::string_view& other) noexcept : _variant(std::string(other)) {}

    DynamicObject& operator=(const DynamicObject& other) {
        _variant = other._variant;
        return *this;
    }

    DynamicObject& operator=(DynamicObject&& other) noexcept {
        _variant = std::move(other._variant);
        return *this;
    }

    bool operator==(const DynamicObject& other) const { return _variant == other._variant; }

    inline DynamicObjectKind kind() const { return static_cast<DynamicObjectKind>(_variant.index()); }

    const std::nullptr_t as_nullptr() const { return rva::get<std::nullptr_t>(_variant); }
    const bool as_bool() const { return rva::get<bool>(_variant); }
    const bool as_double() const { return rva::get<double>(_variant); }
    const std::string& as_string() const { return rva::get<std::string>(_variant); }

    const JsonValue to_json() const;

    const std::string dump_json() const;

    const DynamicObjectVariant& value() const { return _variant; }
    DynamicObjectVariant& value() { return _variant; }

  private:
    DynamicObjectVariant _variant;
};

}; // namespace edgelink