#pragma once

namespace edgelink {

struct INodeDescriptor;
struct IEngine;
class Msg;
struct FlowNode;

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

/// @brief 消息路由器
struct IMsgRouter {

    /// @brief 向路由器里发送产生的第一手消息
    virtual void emit(std::shared_ptr<Msg> msg) = 0;

    /// @brief 生成消息 ID
    /// @return
    virtual uint64_t generate_msg_id() = 0;

    /// @brief 从来源节点向后路由消息
    /// @param src 
    /// @param msg
    virtual void relay(const FlowNode* source, const std::shared_ptr<Msg>& msg, bool clone = true) const = 0;
};

/// @brief 数据处理引擎接口
struct IEngine : public IMsgRouter {
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void run() = 0;
};

/// @brief 节点的发出连接端口
class OutputPort {
public:
  explicit OutputPort(const std::vector<FlowNode*>& wires) : _wires(wires) {}

  const std::vector<FlowNode*>& wires() const { return _wires; }

  static const std::vector<FlowNode*>& EMPTY() {
      static const std::vector<FlowNode*> EMPTY_WIRES(0);
      return EMPTY_WIRES;
  }

private:
  std::vector<FlowNode*> _wires;
};

/// @brief 数据流处理基础元素
struct FlowElement {};

enum class NodeKind {
    JUNCTION = 0, ///< 节点
    SOURCE = 1, ///< 数据源
    SINK = 2,   ///< 数据收集器
    FILTER = 3  ///< 过滤器
};


/// @brief 数据流节点抽象类
class FlowNode : public FlowElement {
  protected:
    FlowNode(uint32_t id, const INodeDescriptor* desc, const std::vector<OutputPort>& output_ports,
             IMsgRouter* router)
        : _id(id), _descriptor(desc), _output_ports(output_ports), _router(router) {
        // constructor
    }

  public:
    inline const std::vector<OutputPort>& output_ports() const { return _output_ports; }
    inline const INodeDescriptor* descriptor() const { return _descriptor; }
    inline IMsgRouter* router() const { return _router; }

    virtual void receive(const std::shared_ptr<Msg>& msg) = 0;

  private:
    const uint32_t _id;
    IMsgRouter* _router;
    const INodeDescriptor* _descriptor;
    const std::vector<OutputPort> _output_ports;

  public:
    virtual void start() = 0;
    virtual void stop() = 0;
};

/// @brief 抽象数据源
class SourceNode :  public FlowNode {
  protected:
    SourceNode(uint32_t id, const INodeDescriptor* desc, const std::vector<OutputPort>& output_ports,
               IMsgRouter* router)
        : FlowNode(id, desc, output_ports, router) {}

  public:
    virtual bool is_running() const { return _thread.joinable(); }
    void start() override; 
    void stop() override;

    void receive(const std::shared_ptr<Msg>& msg) override {
        throw InvalidDataException("错误的流设置：数据源不允许接收数据");
    }

  protected:
    virtual void process(std::stop_token& stoken) = 0;

    std::jthread& thread() { return _thread; }

  private:
    std::jthread _thread; // 一个数据源一个线程
};

/// @brief 抽象数据接收器
class SinkNode : public FlowNode {
  protected:
    SinkNode(uint32_t id, const INodeDescriptor* desc, const std::vector<OutputPort>& output_ports,
             IMsgRouter* router)
        : FlowNode(id, desc, output_ports, router) {}
};

/// @brief 抽象数据过滤器
class FilterNode : public FlowNode {
  protected:
    FilterNode(uint32_t id, const INodeDescriptor* desc, const std::vector<OutputPort>& output_ports,
               IMsgRouter* router)
        : FlowNode(id, desc, output_ports, router) {}
};

struct INodeDescriptor {
    virtual const std::string_view& type_name() const = 0;
    virtual const NodeKind kind() const = 0;
};

struct INodeProvider {
    virtual const INodeDescriptor* descriptor() const = 0;
    virtual FlowNode* create(uint32_t id, const ::nlohmann::json& config, const std::vector<OutputPort>& output_ports,
                             IMsgRouter* router) const = 0;

    RTTR_ENABLE()
};


template <typename TNode, StringLiteral TTypeName, NodeKind TKind>
class NodeProvider final : public INodeProvider, public INodeDescriptor {
  public:
    NodeProvider() : _type_name(TTypeName.value) {}

    const INodeDescriptor* descriptor() const override { return this; }
    const std::string_view& type_name() const override { return _type_name; }
    inline const NodeKind kind() const override { return TKind; }

    FlowNode* create(uint32_t id, const ::nlohmann::json& config, const std::vector<OutputPort>& output_ports,
                     IMsgRouter* router) const override {
        return new TNode(id, config, this, output_ports, router);
    }

  private:
    const std::string_view _type_name;

    RTTR_ENABLE(INodeProvider)
};

}; // namespace edgelink