#include "../../pch.hpp"
#include "edgelink/edgelink.hpp"

using namespace std;

namespace edgelink {

void SourceNode::start() {
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

void SourceNode::stop() {
    if (_thread.joinable()) {
        _thread.request_stop();
        _thread.join();
    }
}

}; // namespace edgelink