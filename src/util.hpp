
//------------------------------------------------------------------------------
// util.hpp
//------------------------------------------------------------------------------

#pragma once

namespace util {
    template<class InputIt, class OutputIt, class Func>
    void transform_out(InputIt first, InputIt last, OutputIt o_first, Func op);
}

template<class InputIt, class OutputIt, class Func>
void util::transform_out(InputIt first, InputIt last, OutputIt o_first, Func op) {
    for (auto& it = first; first != last; first++) {
        op(*it, *o_first);
        o_first++;
    }
}
