#pragma once

#include "edgelink/utils.hpp"

namespace edgelink {

struct IFlow;
struct INodeDescriptor;
struct IEngine;
struct INode;
struct IFlowNode;
struct IStandaloneNode;
struct Envelope;
class OutputPort;

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

/// @brief 路由中的消息封装
struct Envelope : private boost::noncopyable {
    std::shared_ptr<Msg> msg;
    bool clone_message;
    std::string_view source_id;
    IFlowNode* source_node;
    const OutputPort* source_port;
    std::string_view destination_id;
    IFlowNode* destination_node;

    /// @brief 默认构造函数
    Envelope()
        : clone_message(true), source_id(), source_node(nullptr), source_port(0), destination_id(),
          destination_node(nullptr) {}

    /// @brief 构造函数，用于初始化所有成员，并指定源信息
    Envelope(std::shared_ptr<Msg> message, bool clone, const std::string_view& src_id, IFlowNode* src_node,
             const OutputPort* src_port)
        : msg(message), clone_message(clone), source_id(src_id), source_node(src_node), source_port(src_port),
          destination_id(), destination_node(nullptr) {}

    /*
    /// @brief 修改目标节点信息
    void change_destination(IFlowNode* node) {
        BOOST_ASSERT(node != nullptr);
        this->destination_id = node->id();
        this->destination_node = node;
    }
    */
};

using FlowOnSendEvent = boost::signals2::signal<void(IFlow* sender, std::vector<std::unique_ptr<Envelope>>& env)>;
using FlowPreRouteEvent = boost::signals2::signal<void(IFlow* sender, Envelope* env)>;
using FlowPreDeliverEvent = boost::signals2::signal<void(IFlow* sender, Envelope* env)>;
using FlowPostDeliverEvent = boost::signals2::signal<void(IFlow* sender, Envelope* env)>;

using NodeOnReceiveEvent = boost::signals2::signal<Awaitable<void>(IFlowNode* sender, std::shared_ptr<Msg> msg)>;
using NodePostReceiveEvent = boost::signals2::signal<Awaitable<void>(IFlowNode* sender, std::shared_ptr<Msg> msg)>;
using OnDoneEvent = boost::signals2::signal<Awaitable<void>(IFlow* sender, std::shared_ptr<Msg> msg)>;
using OnErrorEvent = boost::signals2::signal<Awaitable<void>(IFlow* sender, std::shared_ptr<Msg> msg)>;

/// @brief 消息流
struct IFlow {

    virtual FlowOnSendEvent& on_send_event() = 0;
    virtual FlowPreRouteEvent& on_pre_route_event() = 0;
    virtual FlowPreDeliverEvent& on_pre_deliver_event() = 0;
    virtual FlowPostDeliverEvent& on_post_deliver_event() = 0;

    virtual const std::string_view id() const = 0;
    virtual const std::string_view label() const = 0;
    virtual bool is_disabled() const = 0;
    virtual IEngine* engine() const = 0;

    virtual Awaitable<void> async_send_many(std::vector<std::unique_ptr<Envelope>>&& envelopes) = 0;

    virtual IFlowNode* get_node(const std::string_view id) const = 0;

    /// @brief 启动流
    /// @return
    virtual Awaitable<void> async_start() = 0;

    /// @brief 停止流
    /// @return
    virtual Awaitable<void> async_stop() = 0;

    // onSend - passed an array of SendEvent objects. The messages inside these objects are exactly what the node has
    // passed to node.send - meaning there could be duplicate references to the same message object.
    // preRoute - called once for each SendEvent object in turn
    // preDeliver - the local router has identified the node it is going to send to. At this point, the message has been
    // cloned if needed. postDeliver - the message has been dispatched to be delivered asynchronously (unless the sync
    // delivery flag is set, in which case it would be continue as synchronous delivery) onReceive - a node is about to
    // receive a message postReceive - the message has been passed to the node's input handler onDone, onError - the
    // node has completed with a message or logged an error
  private:
    RTTR_ENABLE()
};

/// @brief 数据处理引擎接口
struct IEngine {
    virtual Awaitable<void> async_start() = 0;
    virtual Awaitable<void> async_stop() = 0;
    virtual IFlow* get_flow(const std::string_view flow_id) const = 0;
    virtual IStandaloneNode* get_global_node(const std::string_view node_id) const = 0;
    virtual bool is_disabled() const = 0;

  private:
    RTTR_ENABLE()
};

/// @brief 流工厂
struct IFlowFactory {

    virtual std::vector<std::unique_ptr<IFlow>> create_flows(const boost::json::array& flows_config,
                                                             IEngine* engine) const = 0;

    virtual std::vector<std::unique_ptr<IStandaloneNode>> create_global_nodes(const boost::json::array& flows_config,
                                                                              IEngine* engine) const = 0;

private:
    RTTR_ENABLE()
};


/// @brief 节点的发出连接端口
class OutputPort {
  public:
    explicit OutputPort(const std::vector<IFlowNode*>&& wires) : _wires(std::move(wires)) {}
    explicit OutputPort(const std::vector<IFlowNode*>& wires) : _wires(wires) {}

    const std::vector<IFlowNode*>& wires() const { return _wires; }

  private:
    const std::vector<IFlowNode*> _wires;
};

/// @brief 流程处理基础元素
struct IFlowElement {
    virtual const std::string_view id() const = 0;
    virtual const bool is_disabled() const = 0;
    virtual Awaitable<void> async_start() = 0;
    virtual Awaitable<void> async_stop() = 0;
};

enum class NodeKind {
    JUNCTION = 0,   ///< 节点
    STANDALONE = 1, ///< 独立节点
    SOURCE = 2,     ///< 数据源
    SINK = 3,       ///< 数据收集器
    PIPE = 4      ///< 过滤器
};

struct INode : public IFlowElement {
    virtual const std::string_view name() const = 0;
    virtual const std::string_view type() const = 0;
    virtual const INodeDescriptor* descriptor() const = 0;
};

struct IStandaloneNode : public INode {
    virtual IEngine* engine() const = 0;
};

/// @brief 独立节点抽象类
class StandaloneNode : public IStandaloneNode {
  protected:
    StandaloneNode(const std::string_view id, const INodeDescriptor* desc, const boost::json::object& config,
                   IEngine* engine)
        : _logger(spdlog::default_logger()->clone(fmt::format("NODE({}:{})", config.at("type").as_string(), id))),
          _id(id), _type(config.at("type").as_string()), _name(config.at("name").as_string()),
          _disabled(edgelink::json::value_or(config, "d", false)), _descriptor(desc), _engine(engine) {
        // constructor
    }

  public:
    const std::string_view id() const override { return _id; }
    const std::string_view type() const override { return _type; }
    const std::string_view name() const override { return _name; }
    const bool is_disabled() const override { return _disabled; }
    const INodeDescriptor* descriptor() const override { return _descriptor; }
    IEngine* engine() const override { return _engine; };

  protected:
    std::shared_ptr<spdlog::logger> logger() const { return _logger; }

  private:
    std::shared_ptr<spdlog::logger> _logger;
    const std::string _id;
    const std::string _type;
    const std::string _name;
    bool _disabled;
    const INodeDescriptor* _descriptor;
    IEngine* const _engine;

  public:
    virtual Awaitable<void> async_start() = 0;
    virtual Awaitable<void> async_stop() = 0;
};

/// @brief 流程节点抽象类
struct IFlowNode : public INode {

    virtual const std::vector<OutputPort>& output_ports() const = 0;
    virtual const size_t output_count() const = 0;
    virtual IFlow* flow() const = 0;
    virtual Awaitable<void> receive_async(std::shared_ptr<Msg> msg) = 0;
    virtual Awaitable<void> async_send_to_one_port(std::shared_ptr<Msg> msg) = 0;
    virtual Awaitable<void> async_send_to_many_port(std::vector<std::shared_ptr<Msg>>&& msgs) = 0;
};

/// @brief 流程节点基类
class FlowNode : public IFlowNode {
  protected:
    FlowNode(const std::string_view id, const INodeDescriptor* desc, const std::vector<OutputPort>&& output_ports,
             IFlow* flow, const boost::json::object& config)
        : _logger(spdlog::default_logger()->clone(fmt::format("NODE({}:{})", config.at("type").as_string(), id))),
          _id(id), _type(config.at("type").as_string()), _name(config.at("name").as_string()),
          _disabled(edgelink::json::value_or(config, "d", false)), _descriptor(desc),
          _output_ports(std::move(output_ports)), _flow(flow) {
        // constructor
    }

  public:
    const std::string_view id() const override { return _id; }
    const std::string_view type() const override { return _type; }
    const std::string_view name() const override { return _name; }
    const bool is_disabled() const override { return _disabled; }
    const std::vector<OutputPort>& output_ports() const override { return _output_ports; }
    const size_t output_count() const override { return _output_ports.size(); }
    const INodeDescriptor* descriptor() const override { return _descriptor; }
    IFlow* flow() const override { return _flow; }

    Awaitable<void> receive_async(std::shared_ptr<Msg> msg) override;

    Awaitable<void> async_send_to_one_port(std::shared_ptr<Msg> msg) override;

    Awaitable<void> async_send_to_many_port(std::vector<std::shared_ptr<Msg>>&& msgs) override;

  protected:
    std::shared_ptr<spdlog::logger> logger() const { return _logger; };

  private:
    std::shared_ptr<spdlog::logger> _logger;
    const std::string _id;
    const std::string _type;
    const std::string _name;
    bool _disabled;
    IFlow* _flow;
    const INodeDescriptor* _descriptor;
    const std::vector<OutputPort> _output_ports;

  public:
    virtual Awaitable<void> async_start() = 0;
    virtual Awaitable<void> async_stop() = 0;
};

/// @brief 抽象数据源
class SourceNode : public FlowNode {
  protected:
    SourceNode(const std::string_view id, const INodeDescriptor* desc, const std::vector<OutputPort>&& output_ports,
               IFlow* flow, const boost::json::object& config)
        : FlowNode(id, desc, std::move(output_ports), flow, config) {}

  public:
    Awaitable<void> async_start() override {
        // 线程函数
        auto executor = co_await boost::asio::this_coro::executor;

        //auto loop = std::bind(&SourceNode::on_async_run, this);
        boost::asio::co_spawn(executor, this->on_async_run(), boost::asio::detached);
        co_return;
    }

    Awaitable<void> async_stop() { co_return; }

    Awaitable<void> receive_async(std::shared_ptr<Msg> msg) override {
        //
        throw NotSupportedException("错误的流设置：数据源不允许接收数据");
    }

  protected:
    virtual Awaitable<void> on_async_run() = 0;
};

/// @brief 抽象数据接收器节点
class SinkNode : public FlowNode {
  protected:
    SinkNode(const std::string_view id, const INodeDescriptor* desc, const std::vector<OutputPort>&& output_ports,
             IFlow* flow, const boost::json::object& config)
        : FlowNode(id, desc, std::move(output_ports), flow, config) {}
};

/// @brief 全局配置节点
class GlobalConfigNode : public StandaloneNode {
  protected:
    GlobalConfigNode(const std::string_view id, const INodeDescriptor* desc, const boost::json::object& config,
                     IEngine* engine)
        : StandaloneNode(id, desc, config, engine) {}
};

/// @brief 网络端点节点
class EndpointNode : public StandaloneNode {
  protected:
    EndpointNode(const std::string_view id, const INodeDescriptor* desc, const boost::json::object& config,
                 IEngine* engine, const std::string_view host, uint16_t port)
        : StandaloneNode(id, desc, config, engine), _host(host), _port(port) {}

  public:
    const std::string_view host() const { return _host; }
    uint16_t port() const { return _port; }

  private:
    const std::string _host;
    const uint16_t _port;
};

/// @brief 抽象数据过滤器
class PipeNode : public FlowNode {
  protected:
    PipeNode(const std::string_view id, const INodeDescriptor* desc, const std::vector<OutputPort>&& output_ports,
               IFlow* flow, const boost::json::object& config)
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

  private:
    RTTR_ENABLE()
};

struct IFlowNodeProvider : public INodeProvider {
    virtual std::unique_ptr<IFlowNode> create(const std::string_view id, const boost::json::object& config,
                                              const std::vector<OutputPort>&& output_ports, IFlow* flow) const = 0;

  private:
    RTTR_ENABLE(INodeProvider)
};

struct IStandaloneNodeProvider : public INodeProvider {
    virtual std::unique_ptr<IStandaloneNode> create(const std::string_view id, const boost::json::object& config,
                                                    IEngine* engine) const = 0;

  private:
    RTTR_ENABLE(INodeProvider)
};

template <typename TNode, StringLiteral TTypeName, NodeKind TKind>
class FlowNodeProvider final : public IFlowNodeProvider, public INodeDescriptor {
  public:
    FlowNodeProvider() : _type_name(TTypeName.value) {}

    const INodeDescriptor* descriptor() const override { return this; }
    const std::string_view type_name() const override { return _type_name; }
    inline const NodeKind kind() const override { return TKind; }

    std::unique_ptr<IFlowNode> create(const std::string_view id, const boost::json::object& config,
                                      const std::vector<OutputPort>&& output_ports, IFlow* flow) const override {
        return std::make_unique<TNode>(id, config, this, std::move(output_ports), flow);
    }

  private:
    const std::string_view _type_name;

    RTTR_ENABLE(IFlowNodeProvider, INodeDescriptor)
};

template <typename TNode, StringLiteral TTypeName, NodeKind TKind>
class StandaloneNodeProvider final : public IStandaloneNodeProvider, public INodeDescriptor {
  public:
    StandaloneNodeProvider() : _type_name(TTypeName.value) {}

    const INodeDescriptor* descriptor() const override { return this; }
    const std::string_view type_name() const override { return _type_name; }
    inline const NodeKind kind() const override { return TKind; }

    std::unique_ptr<IStandaloneNode> create(const std::string_view id, const boost::json::object& config,
                                            IEngine* engine) const override {
        return std::make_unique<TNode>(id, config, this, engine);
    }

  private:
    const std::string_view _type_name;

    RTTR_ENABLE(IStandaloneNodeProvider, INodeDescriptor)
};

}; // namespace edgelink
