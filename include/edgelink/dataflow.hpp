#pragma once

namespace edgelink {

using MsgValue = rva::variant<          //
    std::nullptr_t,                     // json null
    bool,                               // json boolean
    double,                             // json number
    std::string,                        // json string
    std::map<std::string, rva::self_t>, // json object, type is std::map<std::string, json_value>
    std::vector<rva::self_t>>;          // json array, type is std::vector<json_value>

using MsgPayload = std::map<std::string, MsgValue>;

struct ISourceNode;

struct Msg {
    ISourceNode* source;
    MsgPayload payload;
};

struct IEngine;

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
    virtual void emit(Msg* msg) = 0;
};

/// @brief 数据处理引擎接口
struct IEngine : public IMsgRouter {
    virtual void run() = 0;
};

/// @brief 数据流处理基础元素
struct IDataFlowElement {};

struct IDataFlowNode : public IDataFlowElement {
    virtual void start() = 0;
    virtual void stop() = 0;
};

/// @brief 数据源接口
struct ISourceNode : public IDataFlowNode {
    virtual IMsgRouter* router() const = 0;
};

/// @brief 数据接收器接口
struct ISinkNode : public IDataFlowNode {
    virtual void receive(const Msg* msg) = 0;
};

/// @brief 管道接口
struct IPipe : public IDataFlowElement {
    virtual IDataFlowNode* from() const = 0;
    virtual IDataFlowNode* to() const = 0;

    virtual bool is_match(const Msg* data) const = 0;
};

/// @brief 过滤器接口
struct IFilter : public IDataFlowElement {
    virtual void filter(Msg* msg) const = 0;
};

/// @brief 抽象数据流元素
class AbstractDataFlowElement : public IDataFlowElement {};

/// @brief 抽象数据源
class AbstractSource : public AbstractDataFlowElement, public ISourceNode {
  public:
    AbstractSource(IMsgRouter* router) : _router(router) {}

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
    IMsgRouter* _router;
};

class AbstractPipe : public AbstractDataFlowElement, public IPipe {

  public:
    AbstractPipe(const ::nlohmann::json::object_t& config, IDataFlowNode* from, IDataFlowNode* to)
        : _from(from), _to(to) {}

    IDataFlowNode* from() const override { return _from; }
    IDataFlowNode* to() const override { return _to; }

  private:
    IDataFlowNode* _from;
    IDataFlowNode* _to;
};

class AbstractQueuedSourceNode : public AbstractDataFlowElement, public ISourceNode {};

struct ISourceProvider {
    virtual const std::string_view& type_name() const = 0;
    virtual ISourceNode* create(const ::nlohmann::json& config, IMsgRouter* router) const = 0;

    RTTR_ENABLE()
};

struct ISinkProvider {

    virtual const std::string_view& type_name() const = 0;
    virtual ISinkNode* create(const ::nlohmann::json& config) const = 0;

    RTTR_ENABLE()
};

struct IFilterProvider {

    virtual const std::string_view& type_name() const = 0;
    virtual IFilter* create(const ::nlohmann::json& config) const = 0;

    RTTR_ENABLE()
};

}; // namespace edgelink