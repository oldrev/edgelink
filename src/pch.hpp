#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <span>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <type_traits>
#include <decimal/decimal> /// GCC 拓展?
#include "rva/variant.hpp"

#include <boost/di.hpp>
#include <boost/asio.hpp>
#include <boost/url.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/sync_bounded_queue.hpp>
#include <boost/container/static_vector.hpp>


#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <rttr/registration>
#include <fmt/core.h>



/*
#include "edgelink/edgelink.hpp"
#include "edgelink/logging.hpp"
#include "edgelink/engine.hpp"
#include "edgelink/errors.hpp"
*/