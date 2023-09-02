#pragma once

#include <chrono>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <memory>
#include <queue>
#include <rva/variant.hpp>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>
#include <filesystem>

#include <boost/asio.hpp>
#include <boost/container/static_vector.hpp>
#include <boost/di.hpp>
#include <boost/thread/sync_bounded_queue.hpp>
#include <boost/url.hpp>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <rttr/registration>
#include <rttr/type>
#include <spdlog/spdlog.h>

/*
#include "edgelink/edgelink.hpp"
#include "edgelink/engine.hpp"
#include "edgelink/errors.hpp"
#include "edgelink/logging.hpp"
*/