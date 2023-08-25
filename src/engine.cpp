#include "pch.h"

#include "edgelink/transport/modbus.hpp"

using namespace std;

namespace edgelink {

std::unordered_map<std::string, SourceNodeFactory> Engine::s_source_descriptors;

void Engine::register_source(const std::string& type, const SourceNodeFactory& factory) {
    Engine::s_source_descriptors[type] = factory;
}

void Engine::emit(uint32_t tag, int64_t timestamp, const void* record) {
    // self.emit_stream(tag, [(time, record)])
}

void Engine::match(uint32_t tag) {

    /*
        for m in self._matches:
            if m.match(tag):
                return m
        return None
        */
}

void Engine::run() {
    /*
    for m in self._matches:
        m.start()
    for s in self._sources:
        s.start()
        */
}



}; // namespace edgelink