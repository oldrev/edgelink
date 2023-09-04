#pragma once

#include "edgelink/common.hpp"
#include "edgelink/errors.hpp"
#include "edgelink/utils.hpp"
#include "edgelink/logging.hpp"

#include "edgelink/flow/common.hpp"
#include "edgelink/flow/msg.hpp"
#include "edgelink/flow/dataflow.hpp"

namespace edgelink {

struct IPlugin {};

struct MyPluginClass {
    MyPluginClass() {}
    void perform_calculation() { value += 12; }
    void perform_calculation(int new_value) { value += new_value; }
    int value = 0;

  private:
    RTTR_ENABLE()
};

}; // namespace edgelink