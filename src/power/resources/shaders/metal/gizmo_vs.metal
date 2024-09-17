#include <metal_stdlib>
using namespace metal;

// Vertex Shader
struct VertexOut {
    float4 Position [[position]];
};

vertex VertexOut vertex_main(const device packed_float3 *aPosition [[buffer(0)]],
                             constant float4x4 &aProjection [[buffer(1)]],
                             constant float4x4 &aView [[buffer(2)]],
                             constant float4x4 &aModel [[buffer(3)]],
                             uint id [[vertex_id]]) {
    VertexOut vert;

    float4 worldPosition = aModel * float4(aPosition[id], 1.0);
    vert.Position = aProjection * aView * worldPosition;

    return vert;
}
