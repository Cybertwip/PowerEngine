#include <metal_stdlib>
using namespace metal;

struct FragmentIn {
    float2 texCoord;
};

fragment float4 fragment_main(FragmentIn in [[stage_in]], texture2d<float> texture1 [[texture(0)]], sampler sampler1 [[sampler(0)]]) {
    return texture1.sample(sampler1, in.texCoord);
}
