#pragma once

#include <chrono>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <memory>
#include <queue>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include <boost/asio.hpp>
#include <boost/container/static_vector.hpp>
#include <boost/di.hpp>
#include <boost/url.hpp>
#include <boost/json.hpp>
#include <boost/signals2.hpp>
#include <boost/lexical_cast.hpp>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <rttr/registration>
#include <spdlog/spdlog.h>

#include <modbus/modbus.h>