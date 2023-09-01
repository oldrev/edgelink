#pragma once

namespace edgelink {

struct INodeDescriptor;
struct IEngine;
class Msg;
class Wire;
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

    /// @brief 获取某个指定节点发出的连线
    /// @param node 指定节点
    /// @return 返回该节点发出的所有连线
    virtual const std::vector<const Wire*>& node_wires(const FlowNode* node) const = 0;

    /// @brief 从来源节点向后路由消息
    /// @param src 
    /// @param msg
    virtual void relay(const FlowNode* source, std::shared_ptr<Msg> msg, bool clone = true) const = 0;
};

/// @brief 数据处理引擎接口
struct IEngine : public IMsgRouter {
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void run() = 0;
};

/// @brief 数据流处理基础元素
struct FlowElement {};

enum class NodeKind {
    SOURCE = 0, ///< 数据源
    SINK = 1,   ///< 数据收集器
    FILTER = 2  ///< 过滤器
};

/// @brief 数据流节点抽象类
class FlowNode : public FlowElement {
  protected:
    FlowNode(const INodeDescriptor* desc, IMsgRouter* router) : _descriptor(desc), _router(router) {
        // constructor
    }

  public:
    inline const std::vector<const Wire*>& wires() const { return _router->node_wires(this); }
    inline const INodeDescriptor* descriptor() const { return _descriptor; }
    inline IMsgRouter* router() const { return _router; }

  private:
    IMsgRouter* _router;
    const INodeDescriptor* _descriptor;

  public:
    virtual void start() = 0;
    virtual void stop() = 0;
};

/// @brief 抽象数据源
class SourceNode :  public FlowNode {
  protected:
    SourceNode(const INodeDescriptor* desc, IMsgRouter* router) : FlowNode(desc, router) {}

  public:
    virtual bool is_running() const { return _thread.joinable(); }
    void start() override; 
    void stop() override; 

  protected:
    virtual void process(std::stop_token& stoken) = 0;

    std::jthread& thread() { return _thread; }

  private:
    std::jthread _thread; // 一个数据源一个线程
};

/// @brief 抽象数据接收器
class SinkNode : public FlowNode {
  protected:
    SinkNode(const INodeDescriptor* desc, IMsgRouter* router) : FlowNode(desc, router) {}

  public:
    virtual void receive(const std::shared_ptr<Msg>& msg) = 0;
};

/// @brief 抽象数据过滤器
class FilterNode : public FlowNode {
  protected:
    FilterNode(const INodeDescriptor* desc, IMsgRouter* router) : FlowNode(desc, router) {}

  public:
    virtual void receive(const std::shared_ptr<Msg>& msg) = 0;
};

/// @brief 连接线（边）
class Wire final {
  public:
    Wire(FlowNode* input, FlowNode* output) : _input(input), _output(output) {}

    inline FlowNode* input() const { return _input; }
    inline FlowNode* output() const { return _output; }

  private:
    FlowNode* _input;
    FlowNode* _output;
};

struct INodeDescriptor {
    virtual const std::string_view& type_name() const = 0;
    virtual const NodeKind kind() const = 0;
};

struct INodeProvider {
    virtual const INodeDescriptor* descriptor() const = 0;
    virtual FlowNode* create(const ::nlohmann::json& config, IMsgRouter* router) const = 0;

    RTTR_ENABLE()
};


template <typename TNode, StringLiteral TTypeName, NodeKind TKind>
class NodeProvider final : public INodeProvider, public INodeDescriptor {
  public:
    NodeProvider() : _type_name(TTypeName.value) {}

    const INodeDescriptor* descriptor() const override { return this; }
    const std::string_view& type_name() const override { return _type_name; }
    inline const NodeKind kind() const override { return TKind; }

    FlowNode* create(const ::nlohmann::json& config, IMsgRouter* router) const override {
        return new TNode(config, this, router);
    }

  private:
    const std::string_view _type_name;

    RTTR_ENABLE(INodeProvider)
};

}; // namespace edgelink