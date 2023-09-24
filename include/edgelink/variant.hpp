#pragma once

namespace edgelink {

class Variant;

using VariantObject = std::unordered_map<std::string, Variant>;

using VariantArray = std::vector<Variant>;

class EDGELINK_EXPORT Variant {
  public:
    enum class Kind {
        NULLPTR = 0,
        BOOL,
        INT64,
        DOUBLE,
        STRING,
        BUFFER,
        ARRAY,
        OBJECT,
    };

  public:
    using VariantType = std::variant<std::nullptr_t, bool, int64_t, double, std::string, std::vector<uint8_t>,
                                     VariantArray, VariantObject>;

    Variant() : _data(std::nullptr_t()) {}

    template <typename T> Variant(const T& value) : _data(value) {}

    // Copy constructor
    Variant(const Variant& other) : _data(other._data) {}
    Variant(const VariantObject& other) : _data(other) {}
    Variant(const VariantArray& other) : _data(other) {}

    // Move constructor
    Variant(Variant&& other) noexcept : _data(std::forward<VariantType>(other._data)) { other = std::nullptr_t(); }

    Variant(VariantObject&& other) noexcept : _data(std::forward<VariantType>(other)) {}

    Variant(VariantArray&& other) noexcept : _data(std::forward<VariantType>(other)) {}

    template <typename TValue> Variant& operator=(const TValue& other) {
        _data = other;
        return *this;
    }

    Variant& operator=(const Variant& other) {
        if (this != &other) {
            _data = other._data;
        }
        return *this;
    }

    Variant& operator=(Variant&& other) noexcept {
        if (this != &other) {
            _data = std::move(other._data);
            other._data = std::nullptr_t();
        }
        return *this;
    }

    // Equality and inequality operators
    bool operator==(const Variant& other) const { return _data == other._data; }

    bool operator!=(const Variant& other) const { return _data != other._data; }

    Kind kind() const { return static_cast<Kind>(_data.index()); }

    template <typename T> T const& get() const& { return std::get<T>(_data); }

    template <typename T> void set(const T& value) { _data = value; }

    template <typename T> bool is() const { return std::holds_alternative<T>(_data); }

    VariantArray& as_array() & {
        auto const& self = *this;
        return const_cast<VariantArray&>(self.as_array());
    }

    VariantArray&& as_array() && { return std::move(this->as_array()); }

    VariantArray const& as_array() const& {
        if (this->kind() == Kind::ARRAY) {
            return std::get<VariantArray>(_data);
        }
        throw std::runtime_error("Wrong type: Array required");
    }

    VariantObject& as_object() & {
        auto const& self = *this;
        return const_cast<VariantObject&>(self.as_object());
    }

    VariantObject&& as_object() && { return std::move(this->as_object()); }

    VariantObject const& as_object() const& {
        if (this->kind() == Kind::OBJECT) {
            return std::get<VariantObject>(_data);
        }
        throw std::runtime_error("Wrong type: Object required");
        // detail::throw_system_error( error::not_object, &loc );
    }

    Variant& at(const std::string& key) & { return this->as_object().at(key); }

    Variant&& at(const std::string& key) && { return std::move(std::move(this->as_object()).at(key)); }

    Variant const& at(const std::string& key) const& { return this->as_object().at(key); }

    Variant& at(std::size_t pos) & { return this->as_array().at(pos); }

    Variant&& at(std::size_t pos) && { return std::move(std::move(this->as_array()).at(pos)); }

    Variant const& at(std::size_t pos) const& { return this->as_array().at(pos); }

    Variant const& at_propex(const std::string_view propex) const&;

    Variant& at_propex(const std::string_view propex) & {
        auto const& self = *this;
        return const_cast<Variant&>(self.at_propex(propex));
    }

    Variant&& at_propex(const std::string_view propex) && { return std::move(this->at_propex(propex)); }

    JsonValue to_json() const;

    std::string json_dump() const { return boost::json::serialize(this->to_json()); }

    /*
    Variant& set_at_propex(const std::string_view sv) ;
    Variant* set_at_propex(string_view sv);
    Variant* set_at_propex(string_view sv);
    */

  private:
    VariantType _data;
};

}; // namespace edgelink