#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float2 position [[attribute(0)]];
};

struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
};

struct Uniforms {
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

vertex VertexOut vertex_main(VertexIn in [[stage_in]], constant Uniforms& uniforms [[buffer(0)]]) {
    VertexOut out;
    out.position = uniforms.projection * uniforms.view * uniforms.model * float4(in.position, 0.0, 1.0);
    out.texCoord = in.position.xy;
    return out;
}
