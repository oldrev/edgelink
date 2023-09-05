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
#include <boost/thread/sync_bounded_queue.hpp>
#include <boost/url.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/json.hpp>

#include <signals.h>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <rttr/registration>
#include <rttr/type>
#include <spdlog/spdlog.h>

#include <mqtt/client.h>
