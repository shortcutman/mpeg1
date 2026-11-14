
//------------------------------------------------------------------------------
// image.hpp
//------------------------------------------------------------------------------

#pragma once

#include "colour.hpp"

#include <Metal/Metal.hpp>

#include <cstdint>
#include <ostream>
#include <span>
#include <vector>

namespace metal {

class ImageConversion {
    private:
    NS::SharedPtr<MTL::Device> _device;
    NS::SharedPtr<MTL::Library> _library;
    NS::SharedPtr<MTL::Function> _function;
    NS::SharedPtr<MTL::ComputePipelineState> _computePipelineState;
    NS::SharedPtr<MTL::CommandQueue> _cmdQueue;

public:
    ImageConversion() {}
    ~ImageConversion() {}

    bool setup(NS::SharedPtr<MTL::Device> device);
    void convert(const image::Frame& frame, MTL::Texture* texture);
};

}
