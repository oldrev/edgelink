#pragma once

namespace edgelink {

using FlowElementID = unsigned int;

class EDGELINK_EXPORT FlowConfig {
  public:
    FlowConfig(const boost::json::array& json_array) : _json_array(json_array) {}
    FlowConfig(const FlowConfig& other) : _json_array(other._json_array) {}
    FlowConfig(const FlowConfig&& other) : _json_array(std::move(other._json_array)) {}

    const boost::json::array data() const { return _json_array; }

  private:
    boost::json::array _json_array;
};

using DynamicObject = rva::variant<     ///
    std::nullptr_t,                     ///< null
    bool,                               ///< boolean
    double,                             ///< number
    std::string,                        ///< string
    Bytes,                              ///< bytes buffer
    std::map<std::string, rva::self_t>, ///< object, type is std::map<std::string, json_value>
    std::vector<rva::self_t>>;          ///< array, type is std::vector<json_value>

}; // namespace edgelink