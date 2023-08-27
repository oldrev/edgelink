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
#include "rva/variant.hpp"

#include <boost/di.hpp>
#include <boost/url.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/concurrent_queues/sync_bounded_queue.hpp>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <rttr/registration>


/*
#include "edgelink/edgelink.hpp"
#include "edgelink/logging.hpp"
#include "edgelink/engine.hpp"
#include "edgelink/errors.hpp"
*/