#pragma once


#include "errors.hpp"
#include "dataflow.hpp"
#include "dataflow.pipes.hpp"
#include "logging.hpp"
#include "engine.hpp"

namespace edgelink {

struct IClosable {
    virtual void close() noexcept = 0;
};

struct IDaemonApp {
    virtual void run() = 0;
};

}; // namespace edgelink