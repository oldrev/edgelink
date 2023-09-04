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

    virtual const std::string& id() const = 0;

    /// @brief 向流里发送产生的第一手消息
    virtual Awaitable<void> emit_async(uint32_t source_node_id, std::shared_ptr<Msg> msg) = 0;

    /// @brief 生成消息 ID
    /// @return
    virtual uint64_t generate_msg_id() = 0;

    /// @brief 从来源节点向后路由消息
    /// @param src
    /// @param msg
    virtual Awaitable<void> relay_async(uint32_t source_node_id, std::shared_ptr<Msg> msg, size_t port,
                                        bool clone) const = 0;

    virtual IFlowNode* get_node(uint32_t id) const = 0;

    /// @brief 启动流
    /// @return
    virtual Awaitable<void> start_async() = 0;

    /// @brief 停止流
    /// @return
    virtual Awaitable<void> stop_async() = 0;
};

/// @brief 数据处理引擎接口
struct IEngine : public IFlow {};

/// @brief 流工厂
struct IFlowFactory {
    std::vector<std::unique_ptr<IFlow>> create_flows(const nlohmann::json& flows_config);
};

/// @brief 节点的发出连接端口
class OutputPort {
  public:
    explicit OutputPort(const std::vector<IFlowNode*>&& wires) : _wires(std::move(wires)) {}

    const std::vector<IFlowNode*>& wires() const { return _wires; }

  private:
    const std::vector<IFlowNode*> _wires;
};

/// @brief 数据流处理基础元素
struct FlowElement {};

enum class NodeKind {
    JUNCTION = 0, ///< 节点
    SOURCE = 1,   ///< 数据源
    SINK = 2,     ///< 数据收集器
    FILTER = 3    ///< 过滤器
};

/// @brief 数据流节点抽象类
struct IFlowNode : public FlowElement {
    virtual uint32_t id() const = 0;
    virtual const std::vector<OutputPort>& output_ports() const = 0;
    virtual const size_t output_count() const = 0;
    virtual const INodeDescriptor* descriptor() const = 0;
    virtual IFlow* flow() const = 0;
    virtual Awaitable<void> receive_async(std::shared_ptr<Msg> msg) = 0;
    virtual Awaitable<void> start_async() = 0;
    virtual Awaitable<void> stop_async() = 0;
};

/// @brief 数据流节点抽象类
class FlowNode : public IFlowNode {
  protected:
    FlowNode(uint32_t id, const INodeDescriptor* desc, const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : _id(id), _descriptor(desc), _output_ports(std::move(output_ports)), _flow(flow) {
        // constructor
    }

  public:
    uint32_t id() const override { return _id; }
    const std::vector<OutputPort>& output_ports() const override { return _output_ports; }
    const size_t output_count() const override { return _output_ports.size(); }
    const INodeDescriptor* descriptor() const override { return _descriptor; }
    IFlow* flow() const override { return _flow; }

    virtual Awaitable<void> receive_async(std::shared_ptr<Msg> msg) = 0;

  private:
    const uint32_t _id;
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
    SourceNode(uint32_t id, const INodeDescriptor* desc, const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : FlowNode(id, desc, std::move(output_ports), flow) {}

  public:
    virtual bool is_running() const { return _thread.joinable(); }
    Awaitable<void> start_async() override;
    Awaitable<void> stop_async() override;

    Awaitable<void> receive_async(std::shared_ptr<Msg> msg) override {
        //
        throw InvalidDataException("错误的流设置：数据源不允许接收数据");
    }

    Awaitable<void> work_loop();

  protected:
    virtual Awaitable<void> process_async(std::stop_token& stoken) = 0;
    std::stop_source _stop;

    std::jthread& thread() { return _thread; }

  private:
    std::jthread _thread; // 一个数据源一个线程
};

/// @brief 抽象数据接收器
class SinkNode : public FlowNode {
  protected:
    SinkNode(uint32_t id, const INodeDescriptor* desc, const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : FlowNode(id, desc, std::move(output_ports), flow) {}
};

/// @brief 抽象数据过滤器
class FilterNode : public FlowNode {
  protected:
    FilterNode(uint32_t id, const INodeDescriptor* desc, const std::vector<OutputPort>&& output_ports, IFlow* flow)
        : FlowNode(id, desc, std::move(output_ports), flow) {}
};

struct INodeDescriptor {
    virtual const std::string_view& type_name() const = 0;
    virtual const NodeKind kind() const = 0;

  private:
    RTTR_ENABLE()
};

struct INodeProvider {
    virtual const INodeDescriptor* descriptor() const = 0;
    virtual std::unique_ptr<IFlowNode> create(uint32_t id, const ::nlohmann::json& config,
                                              const std::vector<OutputPort>&& output_ports, IFlow* flow) const = 0;

  private:
    RTTR_ENABLE()
};

template <typename TNode, StringLiteral TTypeName, NodeKind TKind>
class NodeProvider final : public INodeProvider, public INodeDescriptor {
  public:
    NodeProvider() : _type_name(TTypeName.value) {}

    const INodeDescriptor* descriptor() const override { return this; }
    const std::string_view& type_name() const override { return _type_name; }
    inline const NodeKind kind() const override { return TKind; }

    std::unique_ptr<IFlowNode> create(uint32_t id, const ::nlohmann::json& config,
                                      const std::vector<OutputPort>&& output_ports, IFlow* flow) const override {
        return std::make_unique<TNode>(id, config, this, std::move(output_ports), flow);
    }

  private:
    const std::string_view _type_name;

    RTTR_ENABLE(INodeProvider)
};

}; // namespace edgelink