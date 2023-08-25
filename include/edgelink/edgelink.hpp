#pragma once

#include "dataflow.hpp"
#include "logging.hpp"
#include "result.hpp"

namespace edgelink {

struct IClosable {
    virtual void close() noexcept = 0;
};

struct IDaemonApp {
    virtual void run() = 0;
};

struct EdgeLinkSettings {
    std::string project_id;
    std::string device_id;
    std::string server_url;
};

}; // namespace edgelink