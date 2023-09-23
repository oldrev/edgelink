#pragma once

namespace edgelink {

class Variant;

using VariantObject = std::map<std::string, Variant>;
using VariantArray = std::vector<Variant>;

class Variant {
  public:
    enum class Kind { NULLPTR = 0, BOOL, INT64, DOUBLE, STRING, BUFFER, ARRAY, OBJECT };

  public:
    using VariantType = std::variant<std::nullptr_t, bool, int64_t, double, std::string, std::vector<uint8_t>,
                                     VariantArray, VariantObject>;

    Variant() : _data(std::nullptr_t()) {}

    template <typename T> Variant(const T& value) : _data(value) {}

    // Copy constructor
    Variant(const Variant& other) : _data(other._data) {}

    // Move constructor
    Variant(Variant&& other) noexcept : _data(std::forward<VariantType>(other._data)) {
        other._data = std::nullptr_t();
    }

    // Copy assignment operator
    Variant& operator=(const Variant& other) {
        _data = other._data;
        return *this;
    }

    // Move assignment operator
    Variant& operator=(Variant&& other) noexcept {
        _data = std::move(other._data);
        other._data = std::nullptr_t();
        return *this;
    }

    // Equality and inequality operators
    bool operator==(const Variant& other) const { return _data == other._data; }

    bool operator!=(const Variant& other) const { return _data != other._data; }

    Kind kind() const { return static_cast<Kind>(_data.index()); }

    template <typename T> T get() const { return std::get<T>(_data); }

    template <typename T> bool is() const { return std::holds_alternative<T>(_data); }

    template <typename T> void set(const T& value) { _data = value; }

    VariantArray& get_array() { return std::get<VariantArray>(_data); }

    VariantObject& get_object() { return std::get<VariantObject>(_data); }

  private:
    VariantType _data;
};
};