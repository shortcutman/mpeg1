
//------------------------------------------------------------------------------
// util.hpp
//------------------------------------------------------------------------------

#include <vector>

namespace image {
    struct Frame;
}

namespace util {

template<typename... Ts>
std::vector<std::byte> make_bytes(Ts&&... args) noexcept {
    return{std::byte(std::forward<Ts>(args))...};
}

std::vector<std::byte> read_file(const std::string& filename);
bool compare_image(std::string data_filepath, image::Frame& frame);

}
