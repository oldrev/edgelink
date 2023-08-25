#pragma once

namespace edgelink {


/// @brief 数据流处理基础元素
struct IDataFlowElement {
    virtual void start() = 0;
    virtual void stop() = 0;
};

struct IDataFlowNode : public virtual IDataFlowElement {};

struct ISourceNode : public virtual IDataFlowNode {};

struct ISinkNode : public virtual IDataFlowNode {};

struct IFilter : public virtual IDataFlowElement {
    virtual IDataFlowNode& get_input() = 0;
    virtual IDataFlowNode& get_output() = 0;
    virtual bool is_match(const std::span<uint8_t>& data) = 0;
};

class AbstractFilter : public virtual IFilter {

  public:
    AbstractFilter(const ::nlohmann::json::object_t& config, ISourceNode& source, ISinkNode& sink)
        : _source(source), _sink(sink) {}

    virtual IDataFlowNode& get_input() { return _source; }
    virtual IDataFlowNode& get_output() { return _sink; }

  private:
    ISourceNode& _source;
    ISinkNode& _sink;
};

class AbstractQueuedSourceNode : public virtual ISourceNode {};

using SourceNodeFactory = std::function<ISourceNode*(const ::nlohmann::json::object_t&)>;

using SinkNodeFactory = std::function<ISinkNode*(const ::nlohmann::json::object_t&)>;

using FilterNodeFactory = std::function<IFilter*(const ::nlohmann::json::object_t&, ISourceNode*, ISinkNode*)>;


}; // namespace edgelink