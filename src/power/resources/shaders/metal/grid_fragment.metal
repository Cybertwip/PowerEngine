#include <metal_stdlib>
using namespace metal;

fragment float4 fragment_main(float2 texCoord [[stage_in]]) {
    return float4(0.0, 0.0, 0.0, 1.0); // Simple grid color
}
