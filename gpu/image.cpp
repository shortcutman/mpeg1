
//------------------------------------------------------------------------------
// image.cpp
//------------------------------------------------------------------------------

#include "image.hpp"

namespace {
    auto ycbcr_to_rgb_metal = MTLSTR(R"(
#include <metal_stdlib>
using namespace metal;

float adjustAndClamp(float val) {    
    if (val > 255.f) {
        return 1.f;
    } else if (val < 0.f) {
        return 0.f;
    } else {
        return val / 255.f;
    }
}

kernel void ycbcrToRGB(
    texture2d<int, access::read> lumaTex [[ texture(0) ]],
    texture2d<int, access::read> cbTex [[ texture(1) ]],
    texture2d<int, access::read> crTex [[ texture(2) ]],
    texture2d<float, access::write> output [[ texture(3) ]],
    uint2 location [[thread_position_in_grid]],
    uint2 gridSize [[threads_per_grid]]) {

    uint2 adjLoc = uint2(location.x >> 1, location.y >> 1);

    int y = lumaTex.read(location).x - 16;
    int cb = cbTex.read(adjLoc).x - 128;
    int cr = crTex.read(adjLoc).x - 128;

    float4 pixelOut;
    pixelOut.r = adjustAndClamp(1.164f * y + (1.596f * cr));
    pixelOut.g = adjustAndClamp(1.164f * y - (0.813f * cr) - (0.391f * cb));
    pixelOut.b = adjustAndClamp(1.164f * y + (2.018f * cb));
    pixelOut.a = 1.0f;
    output.write(pixelOut, location);
}
    )");
}

bool metal::ImageConversion::setup(NS::SharedPtr<MTL::Device> device) {
    _device = device;

    NS::Error* error = nullptr;
    _library = NS::TransferPtr(_device->newLibrary(ycbcr_to_rgb_metal, nullptr, &error));
    if (!_library) {
        __builtin_printf("Metal library error: %s\n", error->localizedDescription()->utf8String());
        return false;
    }

    _function = NS::TransferPtr(_library->newFunction(MTLSTR("ycbcrToRGB")));
    if (!_function) {

    }

    _computePipelineState = NS::TransferPtr(_device->newComputePipelineState(_function.get(), &error));
    _cmdQueue = NS::TransferPtr(_device->newCommandQueue());

    return true;
}

void metal::ImageConversion::convert(const image::Frame& frame, MTL::Texture* texture) {
    auto luma_txt_desc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
    luma_txt_desc->setPixelFormat(MTL::PixelFormatR32Sint);
    luma_txt_desc->setWidth(frame.luma.width());
    luma_txt_desc->setHeight(frame.luma.height());
    auto luma_buffer = NS::TransferPtr(_device->newBuffer(frame.luma.begin(), frame.luma.buffer_size(), MTL::ResourceStorageModeShared, nullptr));
    auto luma_texture = NS::TransferPtr(luma_buffer->newTexture(luma_txt_desc.get(), 0, frame.luma.width() * 4));

    auto chroma_txt_desc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
    chroma_txt_desc->setPixelFormat(MTL::PixelFormatR32Sint);
    chroma_txt_desc->setWidth(frame.cb.width());
    chroma_txt_desc->setHeight(frame.cb.height());
    auto cb_buffer = NS::TransferPtr(_device->newBuffer(frame.cb.begin(), frame.cb.buffer_size(), MTL::ResourceStorageModeShared, nullptr));
    auto cb_texture = NS::TransferPtr(cb_buffer->newTexture(chroma_txt_desc.get(), 0, frame.cb.width() * 4));
    auto cr_buffer = NS::TransferPtr(_device->newBuffer(frame.cr.begin(), frame.cr.buffer_size(), MTL::ResourceStorageModeShared, nullptr));
    auto cr_texture = NS::TransferPtr(cr_buffer->newTexture(chroma_txt_desc.get(), 0, frame.cr.width() * 4));

    auto commandBuffer = _cmdQueue->commandBuffer();
    auto computeEncoder = commandBuffer->computeCommandEncoder();
    computeEncoder->setComputePipelineState(_computePipelineState.get());
    computeEncoder->setTexture(luma_texture.get(), 0);
    computeEncoder->setTexture(cb_texture.get(), 1);
    computeEncoder->setTexture(cr_texture.get(), 2);
    computeEncoder->setTexture(texture, 3);

    auto grid = MTL::Size(frame.encoded_width, frame.encoded_height, 1);
    auto thread_group = MTL::Size(16, 16, 1);
    computeEncoder->dispatchThreads(grid, thread_group);
    computeEncoder->endEncoding();
    commandBuffer->commit();
    commandBuffer->waitUntilCompleted();
}
