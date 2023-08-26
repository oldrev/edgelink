#pragma once

#include "dataflow.hpp"

namespace edgelink {

class Engine {
  public:
    static void register_source(const ISourceProvider* provider);
    static void register_sink(const ISinkProvider* sink);
    static void register_filter(const IPipeProvider* filter);

  public:
    Engine();
    void run();

  private:
    std::map<std::string, IPipe*> _filters;
    std::map<std::string, ISourceNode*> _sources;
    std::map<std::string, ISinkNode*> _sinks;

    static std::vector<const ISourceProvider*> s_source_providers;
    static std::vector<const IPipeProvider*> s_filter_providers;
    static std::vector<const ISinkProvider*> s_sink_providers;
};

}; // namespace edgelink