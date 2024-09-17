#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position_image [[position]];
    float3 near;
    float3 far;
};

vertex VertexOut vertex_main(const device packed_float2 *const aPosition [[buffer(0)]],
                             constant float4x4 &aView [[buffer(1)]],
                             constant float4x4 &aProjection [[buffer(2)]],
                             constant float4x4 &aInvProjectionView [[buffer(3)]],
                             uint id [[vertex_id]]) {
    VertexOut vert;
    
    float2 p = aPosition[id];

    // Use the precomputed inverse matrix
    auto unproject_point = [&](float x, float y, float z) -> float3 {
        float4 unproj_point = aInvProjectionView * float4(x, y, z, 1.0);
        return unproj_point.xyz / unproj_point.w;
    };

    vert.near = unproject_point(p.x, p.y, -1.0);
    vert.far = unproject_point(p.x, p.y, 1.0);
    
    vert.position_image = float4(p, 0.0, 1.0);

    return vert;
}
