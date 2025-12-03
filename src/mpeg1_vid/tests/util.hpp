
//------------------------------------------------------------------------------
// util.hpp
//------------------------------------------------------------------------------

#include <vector>

namespace util {

template<typename... Ts>
std::vector<std::byte> make_bytes(Ts&&... args) noexcept {
    return{std::byte(std::forward<Ts>(args))...};
}

}
