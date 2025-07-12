
//------------------------------------------------------------------------------
// main.cpp
//------------------------------------------------------------------------------

#include <cstdint>
#include <fstream>
#include <span>
#include <string>
#include <vector>
#include <iostream>

#include "bitspan.hpp"

namespace {

std::vector<std::byte> read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) return {};

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<std::byte> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::print("args are wrong");
    }

    std::string input_filepath = argv[1];

    // std::ifstream input_file(input_filepath, std::ios::binary | std::ios::ate);

    // if (!input_file.is_open()) {
    //     std::println("Unable to open file.");
    //     return -1;
    // }

    std::vector<std::byte> data = read_file(input_filepath);
    // for (auto getbyte = input_file.get(); getbyte == std::ifstream::traits_type::eof(); getbyte = input_file.get()) {
    //     data.push_back(std::byte{static_cast<uint8_t>(getbyte)});
    // }
    auto data_span = std::span{data};

    auto bitspan = util::bitspan(data_span);

    // auto header = bitspan.read_bits(32);
    // std::cout << std::hex << header << std::endl;
    // if ((header >> 24) != 0x47) throw std::runtime_error("expected sync byte");

    // // bitspan.read_bits(1); //tei
    // // bitspan.read_bits(1); //pusi
    // // bitspan.read_bits(1); //pri
    // auto pid = ((header & 0x1fff00) >> 8);
    // std::print("PID: {}", pid);

    auto syncbyte = bitspan.read_bits_be(8);
    if (syncbyte != 0x47) throw std::runtime_error("expected sync byte");

    bitspan.read_bits_be(1); //tei
    bitspan.read_bits_be(1); //pusi
    bitspan.read_bits_be(1); //pri
    auto pid = bitspan.read_bits_be(13);
    std::print("PID: {}", pid);
}