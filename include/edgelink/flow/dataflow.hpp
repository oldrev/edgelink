#pragma once

#include "edgelink/utils.hpp"

namespace edgelink {

struct INodeDescriptor;
struct IEngine;
class IFlowNode;

/// @brief 数据处理上下文
class FlowContext {

  public:
    FlowContext(IEngine* engine, Msg* msg) : _engine(engine), _msg(msg) {}

    inline IEngine* engine() const { return _engine; }
    inline Msg* msg() const { return _msg; }

  private:
    IEngine* _engine;
    Msg* _msg;
};

/// @brief 消息流
struct IFlow {

    virtual const std::string_view id() const = 0;
    virtual const std::string_view name() const = 0;
    virtual bool is_disabled() const = 0;

    /// @brief 向流里发送产生的第一手消息
    virtual Awaitable<void> emit_async(FlowNodeID source_node_id, std::shared_ptr<Msg> msg) = 0;

    /// @brief 从来源节点向后路由消息
    /// @param src
    /// @param msg
    virtual Awaitable<void> relay_async(FlowNodeID source_node_id, std::shared_ptr<Msg> msg, size_t port,
                                        bool clone) const = 0;

    virtual IFlowNode* get_node(FlowNodeID id) const = 0;

    /// @brief 启动流
    /// @return
    virtual Awaitable<void> start_async() = 0;

    /// @brief 停止流
    /// @return
    virtual Awaitable<void> stop_async() = 0;

    // onSend - passed an array of SendEvent objects. The messages inside these objects are exactly what the node has
    // passed to node.send - meaning there could be duplicate references to the same message object.
    // preRoute - called once for each SendEvent object in turn
    // preDeliver - the local router has identified the node it is going to send to. At this point, the message has been
    // cloned if needed. postDeliver - the message has been dispatched to be delivered asynchronously (unless the sync
    // delivery flag is set, in which case it would be continue as synchronous delivery) onReceive - a node is about to
    // receive a message postReceive - the message has been passed to the node's input handler onDone, onError - the
    // node has completed with a message or logged an error
};

/// @brief 数据处理引擎接口
struct IEngine {
    virtual Awaitable<void> start_async() = 0;
    virtual Awaitable<void> stop_async() = 0;
    virtual IFlow* get_flow(const std::string_view flow_id) const = 0;
    virtual bool is_disabled() const = 0;
};

/// @brief 流工厂
struct IFlowFactory {
    virtual std::vector<std::unique_ptr<IFlow>> create_flows(const boost::json::array& flows_config) const = 0;
};

/// @brief 节点的发出连接端口
class OutputPort {
  public:
    explicit OutputPort(const std::vector<IFlowNode*>&& wires) : _wires(std::move(wires)) {}

    const std::vector<IFlowNode*>& wires() const { return _wires; }

  private:
    const std::vector<IFlowNode*> _wires;
};

/// @brief 流程处理基础元素
struct FlowElement {};

enum class NodeKind {
    JUNCTION = 0,   ///< 节点
    STANDALONE = 1, ///< 独立节点
    SOURCE = 2,     ///< 数据源
    SINK = 3,       ///< 数据收集器
    FILTER = 4      ///< 过滤器
};

/// @brief 流程节点抽象类
struct IFlowNode : public FlowElement {
    virtual FlowNodeID id() const = 0;
    virtual const std::string_view name() const = 0;
    virtual const bool is_disabled() const = 0;
    virtual const std::vector<OutputPort>& output_ports() const = 0;
    virtual const size_t output_count() const = 0;
    virtual const INodeDescriptor* descriptor() const = 0;
    virtual IFlow* flow() const = 0;
    virtual Awaitable<void> receive_async(std::shared_ptr<Msg> msg) = 0;
    virtual Awaitable<void> start_async() = 0;
    virtual Awaitable<void> stop_async() = 0;
};

/// @brief 流程节点抽象类
class FlowNode : public IFlowNode {
  protected:
    FlowNode(FlowNodeID id, const INodeDescriptor* desc, const std::vector<OutputPort>&& output_ports, IFlow* flow,
             const boost::json::object& config)
        : _id(id), _name(config.at("name").as_string()), _disabled(edgelink::json::value_or(config, "d", false)),
          _descriptor(desc), _output_ports(std::move(output_ports)), _flow(flow) {
        // constructor
    }

  public:
    FlowNodeID id() const override { return _id; }
    const std::string_view name() const override { return _name; }
    const bool is_disabled() const override { return _disabled; }
    const std::vector<OutputPort>& output_ports() const override { return _output_ports; }
    const size_t output_count() const override { return _output_ports.size(); }
    const INodeDescriptor* descriptor() const override { return _descriptor; }
    IFlow* flow() const override { return _flow; }

    virtual Awaitable<void> receive_async(std::shared_ptr<Msg> msg) = 0;

  private:
    const FlowNodeID _id;
    const std::string _name;
    bool _disabled;
    IFlow* _flow;
    const INodeDescriptor* _descriptor;
    const std::vector<OutputPort> _output_ports;

  public:
    virtual Awaitable<void> start_async() = 0;
    virtual Awaitable<void> stop_async() = 0;
};

/// @brief 抽象数据源
class SourceNode : public FlowNode {
  protected:
    SourceNode(FlowNodeID id, const INodeDescriptor* desc, const std::vector<OutputPort>&& output_ports, IFlow* flow,
               const boost::json::object& config)
        : FlowNode(id, desc, std::move(output_ports), flow, config) {}

  public:
    Awaitable<void> start_async() override;
    Awaitable<void> stop_async() override;

    Awaitable<void> receive_async(std::shared_ptr<Msg> msg) override {
        //
        throw InvalidDataException("错误的流设置：数据源不允许接收数据");
    }

  protected:
    virtual Awaitable<void> process_async(std::stop_token& stoken) = 0;
    std::stop_source _stop;

  private:
    Awaitable<void> work_loop();
};

/// @brief 抽象数据接收器节点
class SinkNode : public FlowNode {
  protected:
    SinkNode(FlowNodeID id, const INodeDescriptor* desc, const std::vector<OutputPort>&& output_ports, IFlow* flow,
             const boost::json::object& config)
        : FlowNode(id, desc, std::move(output_ports), flow, config) {}
};

/// @brief 独立节点
class StandaloneNode : public FlowNode {
  protected:
    StandaloneNode(FlowNodeID id, const INodeDescriptor* desc, const std::vector<OutputPort>&& output_ports,
                   IFlow* flow, const boost::json::object& config)
        : FlowNode(id, desc, std::move(output_ports), flow, config) {}
};

/// @brief 全局配置节点
class GlobalConfigNode : public StandaloneNode {
  protected:
    GlobalConfigNode(FlowNodeID id, const INodeDescriptor* desc, const std::vector<OutputPort>&& output_ports,
                     IFlow* flow, const boost::json::object& config)
        : StandaloneNode(id, desc, std::move(output_ports), flow, config) {}
};

/// @brief 网络端点节点
class EndpointNode : public StandaloneNode {
  protected:
    EndpointNode(FlowNodeID id, const INodeDescriptor* desc, const std::vector<OutputPort>&& output_ports, IFlow* flow,
                 const boost::json::object& config, const std::string_view host, uint16_t port)
        : StandaloneNode(id, desc, std::move(output_ports), flow, config), _host(host), _port(port) {}

  public:
    const std::string_view host() const { return _host; }
    uint16_t port() const { return _port; }

  private:
    const std::string _host;
    const uint16_t _port;
};

/// @brief 抽象数据过滤器
class FilterNode : public FlowNode {
  protected:
    FilterNode(FlowNodeID id, const INodeDescriptor* desc, const std::vector<OutputPort>&& output_ports, IFlow* flow,
               const boost::json::object& config)
        : FlowNode(id, desc, std::move(output_ports), flow, config) {}
};

struct INodeDescriptor {
    virtual const std::string_view type_name() const = 0;
    virtual const NodeKind kind() const = 0;

  private:
    RTTR_ENABLE()
};

struct INodeProvider {
    virtual const INodeDescriptor* descriptor() const = 0;
    virtual std::unique_ptr<IFlowNode> create(FlowNodeID id, const boost::json::object& config,
                                              const std::vector<OutputPort>&& output_ports, IFlow* flow) const = 0;

  private:
    RTTR_ENABLE()
};

template <typename TNode, StringLiteral TTypeName, NodeKind TKind>
class NodeProvider final : public INodeProvider, public INodeDescriptor {
  public:
    NodeProvider() : _type_name(TTypeName.value) {}

    const INodeDescriptor* descriptor() const override { return this; }
    const std::string_view type_name() const override { return _type_name; }
    inline const NodeKind kind() const override { return TKind; }

    std::unique_ptr<IFlowNode> create(FlowNodeID id, const boost::json::object& config,
                                      const std::vector<OutputPort>&& output_ports, IFlow* flow) const override {
        return std::make_unique<TNode>(id, config, this, std::move(output_ports), flow);
    }

  private:
    const std::string_view _type_name;

    RTTR_ENABLE(INodeProvider)
};
}; // namespace edgelink