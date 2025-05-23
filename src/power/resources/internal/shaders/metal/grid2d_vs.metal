#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float4 scroll;
};

vertex VertexOut vertex_main(const device packed_float2 *const aPosition [[buffer(0)]],
                             constant float2 &scrollOffset [[buffer(1)]],
                             constant float4x4 &aView [[buffer(2)]],
                             constant float4x4 &aProjection [[buffer(3)]],
                             uint id [[vertex_id]]) {
    VertexOut out;
    float2 pos = aPosition[id];
    out.position = aProjection * aView * float4(pos, 0.0, 1.0);
    out.scroll = aProjection * aView * float4(scrollOffset, 0.0, 1.0);
    return out;
}
