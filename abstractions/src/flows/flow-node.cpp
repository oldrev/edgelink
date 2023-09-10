
#include "edgelink/edgelink.hpp"

namespace edgelink {

Awaitable<void> FlowNode::async_send_to_one_port(std::shared_ptr<Msg> msg) {
    if (this->output_ports().size() != 1) {
        this->logger()->warn("节点 (id={}, name={}) 必须只有一个端口才能调用本方法", this->id(), this->name());
        co_return;
    }

    bool msg_sent = false;
    auto const& port = this->output_ports().front();
    for (auto const& dest_node : port.wires()) {
        Envelope env(msg, msg_sent, this->id(), this, &port);
        env.clone_message = msg_sent;
        env.destination_id = dest_node->id();
        env.destination_node = dest_node;

        co_await this->flow()->async_send_one(std::forward<Envelope>(env));

        msg_sent = true;
    }
    co_return;
}

Awaitable<void> FlowNode::async_send_to_many_port(std::vector<std::shared_ptr<Msg>>&& msgs) {
    auto const& ports = this->output_ports();
    if (msgs.size() > ports.size()) {
        auto error_msg = "发送的消息超出端口数量";
        this->logger()->error(error_msg);
        throw std::out_of_range(error_msg);
    }
    std::vector<Envelope> envelopes;
    bool msg_sent = false;
    for (size_t iport = 0; iport < msgs.size(); iport++) {
        auto const& port = &ports.at(iport);
        for (size_t iwire = 0; iwire < ports.size(); iwire++) {

            auto const& dest_node = port->wires().at(iwire);

            Envelope env(msgs[iport], msg_sent, this->id(), this, port);
            env.clone_message = msg_sent;
            env.destination_id = dest_node->id();
            env.destination_node = dest_node;

            envelopes.emplace_back(env);
            msg_sent = true;
        }
    }
    co_return;
}

}; // namespace edgelink