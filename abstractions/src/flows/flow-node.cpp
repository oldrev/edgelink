
#include "edgelink/edgelink.hpp"

namespace edgelink {

Awaitable<void> FlowNode::async_send_to_one_port(std::shared_ptr<Msg> msg) {
    if (this->output_ports().size() != 1) {
        this->logger()->warn("节点 (id={}, name={}) 必须只有一个端口才能调用本方法", this->id(), this->name());
        co_return;
    }

    std::vector<std::shared_ptr<Msg>> envelopes;
    
    envelopes.emplace_back(msg);

    co_await this->async_send_to_many_port(std::move(envelopes));
    co_return;
}

Awaitable<void> FlowNode::async_send_to_many_port(std::vector<std::shared_ptr<Msg>>&& msgs) {
    auto&& ports = this->output_ports();
    if (msgs.size() > ports.size()) {
        auto error_msg = "发送的消息超出端口数量";
        this->logger()->error(error_msg);
        throw std::out_of_range(error_msg);
    }
    std::vector<std::unique_ptr<Envelope>> envelopes;
    bool msg_sent = false;
    for (size_t iport = 0; iport < msgs.size(); iport++) {
        auto&& port = &ports.at(iport);
        for (size_t iwire = 0; iwire < port->wires().size(); iwire++) {

            IFlowNode* dest_node = port->wires().at(iwire);

            auto env = std::make_unique<Envelope>(msgs[iport], msg_sent, this->id(), this, port);
            env->destination_id = dest_node->id();
            env->destination_node = dest_node;

            envelopes.emplace_back(std::move(env));
            msg_sent = true;
        }
    }
    co_await this->flow()->async_send_many(std::move(envelopes));
    co_return;
}

}; // namespace edgelink