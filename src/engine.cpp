#include <map>
#include <memory>
#include <span>
#include <string>
#include <nlohmann/json.hpp>

#include "edgelink/edgelink.hpp"
#include "edgelink/engine.hpp"
#include "edgelink/transport/modbus.hpp"

using namespace std;

namespace edgelink {

std::unordered_map<std::string, SourceNodeFactory> Engine::s_source_descriptors;

void Engine::register_source(const std::string& type, const SourceNodeFactory& factory) {
    Engine::s_source_descriptors[type] = factory;
}

}; // namespace edgelink