#pragma once

#include <boost/system/result.hpp>
#include <system_error>

namespace edgelink {

struct EmptyResult {};

template <class T = EmptyResult> using Result = boost::system::result<T, std::error_code>;

#define TRY(...)                                                                                                       \
    ({                                                                                                                 \
        auto _temporary_result = __VA_ARGS__;                                                                          \
        if (!_temporary_result.has_error()) {                                                                          \
            return _temporary_result.error();                                                                          \
        }                                                                                                              \
        _temporary_result.value();                                                                                     \
    })

}; // namespace edgelink