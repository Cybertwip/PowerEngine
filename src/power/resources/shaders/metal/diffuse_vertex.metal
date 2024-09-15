#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 fragPos;
    float3 normal;
};

struct Uniforms {
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

vertex VertexOut vertex_main(VertexIn in [[stage_in]], constant Uniforms& uniforms [[buffer(0)]]) {
    VertexOut out;
    out.fragPos = (uniforms.model * float4(in.position, 1.0)).xyz;
    out.normal = (float3x3(transpose(inverse(uniforms.model))) * in.normal);
    
    out.position = uniforms.projection * uniforms.view * uniforms.model * float4(in.position, 1.0);
    return out;
}
