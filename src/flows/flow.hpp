#pragma once

#include "edgelink/utils.hpp"

namespace edgelink {
class FlowNode;
struct IRegistry;
struct IEngine;
}; // namespace edgelink

namespace edgelink::flow::details {

class Flow : public IFlow {

  public:
    Flow(const boost::json::object& json_config, IEngine* engine);
    virtual ~Flow();

    const std::string_view id() const override { return _id; }
    const std::string_view label() const override { return _label; }
    bool is_disabled() const override { return _disabled; }
    IEngine* engine() const override { return _engine; }

    Awaitable<void> start_async() override;
    Awaitable<void> stop_async() override;

    Awaitable<void> emit_async(const std::string_view source_node_id, std::shared_ptr<Msg> msg) override;

    Awaitable<void> relay_async(const std::string_view source_node_id, std::shared_ptr<Msg> msg, size_t port,
                                bool clone) const override;

    inline IFlowNode* get_node(const std::string_view id) const override {

        for (auto& n : _nodes) {
            if (n->id() == id) {
                return n.get();
            }
        }
        return nullptr;
    }

    inline void emplace_node(std::unique_ptr<IFlowNode>&& node) { _nodes.emplace_back(std::move(node)); }

  private:
    std::shared_ptr<spdlog::logger> _logger;
    const std::string _id;
    const std::string _label;
    const bool _disabled;
    IEngine*const _engine;
    std::vector<std::unique_ptr<IFlowNode>> _nodes;

    std::atomic<uint64_t> _msg_id_counter; // 初始化计数器为0

    std::unique_ptr<std::stop_source> _stop_source;
};

}; // namespace edgelink::flow::details