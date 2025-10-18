
//------------------------------------------------------------------------------
// decode.hpp
//------------------------------------------------------------------------------

#pragma once

#include <optional>
#include <span>
#include <vector>

namespace util {
    class bitspan;
}

namespace mpeg1 {
    std::optional<uint32_t> get_code(const std::span<std::byte>& data);
    bool peak_code(const util::bitspan& data);

    void decode(std::vector<std::byte>& data);
}
