#pragma once

namespace edgelink {

using MsgValue = rva::variant<          //
    std::nullptr_t,                     // json null
    bool,                               // json boolean
    double,                             // json number
    std::string,                        // json string
    std::map<std::string, rva::self_t>, // json object, type is std::map<std::string, json_value>
    std::vector<rva::self_t>>;          // json array, type is std::vector<json_value>

using Msg = std::map<std::string, MsgValue>;

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

/// @brief 数据处理引擎接口
struct IEngine {
    virtual void emit(Msg* msg) = 0;
};

/// @brief 数据流处理基础元素
struct IDataFlowElement {};

struct IDataFlowNode : public virtual IDataFlowElement {
    virtual void start() = 0;
    virtual void stop() = 0;
};

/// @brief 数据源接口
struct ISourceNode : public virtual IDataFlowNode {};

/// @brief 数据接收器接口
struct ISinkNode : public virtual IDataFlowNode {};

/// @brief 管道接口
struct IPipe : public virtual IDataFlowElement {
    virtual IDataFlowElement* from() const = 0;
    virtual IDataFlowElement* to() const = 0;

    virtual bool is_match(const Msg* data) const = 0;
};

/// @brief 过滤器接口
struct IFilter : public virtual IDataFlowElement {
    virtual void emit(Msg* msg) const = 0;
};

/// @brief 抽象数据流元素
class AbstractDataFlowElement : public virtual IDataFlowElement {};

/// @brief 抽象数据源
class AbstractSource : public virtual AbstractDataFlowElement, public virtual ISourceNode {
  public:
    void start() override {
        _thread = std::jthread([this](std::stop_token stoken) {
            // 线程函数
            while (!stoken.stop_requested()) {
                this->process(stoken);
                // std::cout << "Thread is running..." << std::endl;
                // std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
    }

    void stop() override {
        _thread.request_stop();
        _thread.join();
    }

  protected:
    virtual void process(std::stop_token& stoken) = 0;

    std::jthread& thread() { return _thread; }
    // boost::concurrent::concurrent_queue<Msg*>& msg_queue() { return _msg_queue; }

    void emit_msg(Msg* msg) {
        // 直接将一个消息入队
        //_msg_queue.push(msg);
    }

  private:
    std::jthread _thread; // 一个数据源一个线程
    // boost::concurrent::concurrent_queue<Msg*> _msg_queue;
};

class AbstractPipe : public virtual AbstractDataFlowElement, public virtual IPipe {

  public:
    AbstractPipe(const ::nlohmann::json::object_t& config, IDataFlowElement* from, IDataFlowElement* to)
        : _from(from), _to(to) {}

    IDataFlowElement* from() const { return _from; }
    IDataFlowElement* to() const { return _to; }

  private:
    IDataFlowElement* _from;
    IDataFlowElement* _to;
};

class AbstractQueuedSourceNode : public virtual AbstractDataFlowElement, public virtual ISourceNode {};

struct ISourceProvider {
    virtual const std::string& type_name() const = 0;
    virtual ISourceNode* create(const ::nlohmann::json::object_t& config) const = 0;
};

struct ISinkProvider {
    virtual const std::string& type_name() const = 0;
    virtual ISinkNode* create(const ::nlohmann::json::object_t& config) const = 0;
};

struct IPipeProvider {
    virtual const std::string& type_name() const = 0;
    virtual IPipe* create(const ::nlohmann::json::object_t& config, IDataFlowElement* source,
                          IDataFlowElement* sink) const = 0;
};

}; // namespace edgelink