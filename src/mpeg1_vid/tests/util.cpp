
//------------------------------------------------------------------------------
// util.cpp
//------------------------------------------------------------------------------

#include "util.hpp"

#include "colour.hpp"

#include <fstream>
#include <sstream>

std::vector<std::byte> util::read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) return {};

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<std::byte> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

bool util::compare_image(std::string data_filepath, image::Frame& frame) {
    std::stringstream ss;
    image::writeOutPPM(ss, frame);
    auto contents = ss.str();

    auto expected = read_file(data_filepath);

    return std::equal(contents.begin(), contents.end(), expected.begin(),
        [] (char a, std::byte b) {
            return a == std::to_integer<char>(b);
        });
}
