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

/// @brief 数据流处理基础元素
struct IDataFlowElement {
    virtual void start() = 0;
    virtual void stop() = 0;
};

struct IDataFlowNode : public virtual IDataFlowElement {};

struct ISourceNode : public virtual IDataFlowNode {};

struct ISinkNode : public virtual IDataFlowNode {};

struct IFilter : public virtual IDataFlowElement {
    virtual IDataFlowElement* get_input() = 0;
    virtual IDataFlowElement* get_output() = 0;
    virtual bool is_match(const std::span<uint8_t>& data) const = 0;
};

class AbstractSource : public virtual ISourceNode {
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
    boost::concurrent::concurrent_queue<Msg*>& msg_queue() { return _msg_queue; }

    void emit_msg(Msg* msg) {
        // 直接将一个消息入队
        _msg_queue.push(msg);
    }

  private:
    std::jthread _thread; // 一个数据源一个线程
    boost::concurrent::concurrent_queue<Msg*> _msg_queue;
};

class AbstractFilter : public virtual IFilter {

  public:
    AbstractFilter(const ::nlohmann::json::object_t& config, ISourceNode* source, ISinkNode* sink)
        : _source(source), _sink(sink) {}

    virtual IDataFlowElement* get_input() { return _source; }
    virtual IDataFlowElement* get_output() { return _sink; }

  private:
    ISourceNode* _source;
    ISinkNode* _sink;
};

class AbstractQueuedSourceNode : public virtual ISourceNode {};

struct ISourceProvider {
    virtual const std::string& type_name() const = 0;
    virtual ISourceNode* create(const ::nlohmann::json::object_t& config) const = 0;
};

struct ISinkProvider {
    virtual const std::string& type_name() const = 0;
    virtual ISinkNode* create(const ::nlohmann::json::object_t& config) const = 0;
};

struct IFilterProvider {
    virtual const std::string& type_name() const = 0;
    virtual IFilter* create(const ::nlohmann::json::object_t& config, ISourceNode* source, ISinkNode* sink) const = 0;
};

}; // namespace edgelink