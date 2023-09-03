#include "edgelink/edgelink.hpp"

using namespace std;
using namespace boost;
namespace this_coro = boost::asio::this_coro;

namespace edgelink {

Awaitable<void> SourceNode::start_async() {
    // 线程函数

    auto executor = co_await this_coro::executor;

    auto loop = std::bind(&SourceNode::work_loop, this);
    asio::co_spawn(executor, loop, asio::detached);
    co_return;
}

Awaitable<void> SourceNode::stop_async() { co_return; }

Awaitable<void> SourceNode::work_loop() {
    auto stoken = _stop.get_token();
    while (!_stop.stop_requested()) {
        co_await this->process_async(stoken);
    }
    co_return;
}

}; // namespace edgelink