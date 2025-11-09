
//------------------------------------------------------------------------------
// util.hpp
//------------------------------------------------------------------------------

#pragma once

#include <cstddef>
#include <span>

namespace util {
    template<class InputIt, class OutputIt, class Func>
    void transform_out(InputIt first, InputIt last, OutputIt o_first, Func op);

    class bitspan;

    class ByteCatcha {
    private:
        size_t _bits_read_start;
        std::span<std::byte> _data;
        util::bitspan& _bits;

    public:
        ByteCatcha(util::bitspan& data);
        ~ByteCatcha();
    };
}

template<class InputIt, class OutputIt, class Func>
void util::transform_out(InputIt first, InputIt last, OutputIt o_first, Func op) {
    for (auto& it = first; first != last; first++) {
        op(*it, *o_first);
        o_first++;
    }
}
