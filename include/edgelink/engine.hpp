#pragma once

#include "dataflow.hpp"

namespace edgelink {

class Engine {
  public:
    static void register_source(const std::string& type, const SourceNodeFactory& factory);

  private:
    std::map<std::string, IFilter*> _filters;
    std::map<std::string, ISourceNode*> _sources;
    std::map<std::string, ISinkNode*> _sinks;

    static std::unordered_map<std::string, SourceNodeFactory> s_source_descriptors;
};

}; // namespace edgelink