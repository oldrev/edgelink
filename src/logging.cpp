#include <iostream>
#include <string>

#include <boost/format.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>

#include "edgelink/logging.hpp"

namespace logging = boost::log;
namespace keywords = boost::log::keywords;

namespace edgelink {

void init_logging() {

    // 设置日志记录级别为DEBUG
    logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::info);

    // 创建控制台输出器，并设置格式
    logging::add_console_log(std::cout, keywords::format = "[%TimeStamp%] [%Severity%]: %Message%");

    // 创建文件输出器，并设置格式
    logging::add_file_log(keywords::file_name = "edgelink.log",
                          keywords::format = "[%TimeStamp%] [%Severity%]: %Message%");
}


}; // namespace edgelink