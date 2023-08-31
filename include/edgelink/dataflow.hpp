#pragma once

namespace edgelink {

struct INodeDescriptor;
struct IEngine;
class Msg;


/// @brief 数据处理上下文
class DataFlowContext {

  public:
    DataFlowContext(IEngine* engine, Msg* msg) : _engine(engine), _msg(msg) {}

    inline IEngine* engine() const { return _engine; }
    inline Msg* msg() const { return _msg; }

  private:
    IEngine* _engine;
    Msg* _msg;
};

/// @brief 消息路由器
struct IMsgRouter {
    virtual void emit(std::shared_ptr<Msg> msg) = 0;
    virtual uint64_t generate_msg_id() = 0;
};

/// @brief 数据处理引擎接口
struct IEngine : public IMsgRouter {
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void run() = 0;
};

/// @brief 数据流处理基础元素
struct IDataFlowElement {};

enum class NodeKind {
    SOURCE = 0, ///< 数据源
    SINK = 1,   ///< 数据收集器
    FILTER = 2  ///< 过滤器
};

struct IDataFlowNode : public IDataFlowElement {
    virtual const INodeDescriptor* descriptor() const = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual IMsgRouter* router() const = 0;
};

/// @brief 数据源接口
struct ISourceNode : public IDataFlowNode {};

/// @brief 数据接收器接口
struct ISinkNode : public IDataFlowNode {
    virtual void receive(const std::shared_ptr<Msg>& msg) = 0;
};

/// @brief 过滤器接口
struct IFilter : public IDataFlowNode {
    virtual std::shared_ptr<Msg> filter(std::shared_ptr<Msg> msg) = 0;
};

/// @brief 抽象数据源
class AbstractSource : public ISourceNode {
  public:
    AbstractSource(const INodeDescriptor* desc, IMsgRouter* router) : _descriptor(desc), _router(router) {}

    const INodeDescriptor* descriptor() const override { return _descriptor; }
    IMsgRouter* router() const override { return _router; }

    void start() override {
        if (!_thread.joinable()) {
            _thread = std::jthread([this](std::stop_token stoken) {
                // 线程函数
                while (!stoken.stop_requested()) {
                    this->process(stoken);
                    // std::cout << "Thread is running..." << std::endl;
                    // std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            });
        }
    }

    void stop() override {
        if (_thread.joinable()) {
            _thread.request_stop();
            _thread.join();
        }
    }

    virtual bool is_running() const { return _thread.joinable(); }

  protected:
    virtual void process(std::stop_token& stoken) = 0;

    std::jthread& thread() { return _thread; }

    void emit_msg(Msg* msg) {
        // 直接将一个消息入队
        //_msg_queue.push(msg);
    }

  private:
    std::jthread _thread; // 一个数据源一个线程

  private:
    IMsgRouter* _router;
    const INodeDescriptor* _descriptor;
};

/// @brief 抽象数据接收器
class AbstractSink : public ISinkNode {
  public:
    AbstractSink(const INodeDescriptor* desc, IMsgRouter* router) : _descriptor(desc), _router(router) {}

    const INodeDescriptor* descriptor() const override { return _descriptor; }
    IMsgRouter* router() const override { return _router; }

  private:
    IMsgRouter* _router;
    const INodeDescriptor* _descriptor;
};

class Pipe final {

  public:
    Pipe(IDataFlowNode* input, IDataFlowNode* output) : _input(input), _output(output) {}

    IDataFlowNode* input() const { return _input; }
    IDataFlowNode* output() const { return _output; }

  private:
    IDataFlowNode* _input;
    IDataFlowNode* _output;
};

struct INodeDescriptor {
    virtual const std::string_view& type_name() const = 0;
    virtual const NodeKind kind() const = 0;
};

struct INodeProvider {
    virtual const INodeDescriptor* descriptor() const = 0;
    virtual IDataFlowNode* create(const ::nlohmann::json& config, IMsgRouter* router) const = 0;

    RTTR_ENABLE()
};


template <typename TNode, StringLiteral TTypeName, NodeKind TKind>
class NodeProvider : public INodeProvider, public INodeDescriptor {
  public:
    NodeProvider() : _type_name(TTypeName.value) {}

    const INodeDescriptor* descriptor() const override { return this; }
    const std::string_view& type_name() const override { return _type_name; }
    inline const NodeKind kind() const override { return TKind; }

    IDataFlowNode* create(const ::nlohmann::json& config, IMsgRouter* router) const override {
        return new TNode(config, this, router);
    }

  private:
    const std::string_view _type_name;

    RTTR_ENABLE(INodeProvider)
};

}; // namespace edgelink