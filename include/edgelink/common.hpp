#pragma once

namespace edgelink {

struct IClosable {
    virtual void close() noexcept = 0;
};

}; // namespace edgelink
