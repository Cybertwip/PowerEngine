#include <metal_stdlib>
using namespace metal;

// Helper function to unproject a point from clip space to world space
float3 unproject_point(float x, float y, float z, constant float4x4 &invProjView) {
    float4 unproj_point = invProjView * float4(x, y, z, 1.0);
    return unproj_point.xyz / unproj_point.w;
}

struct VertexOut {
    float4 position [[position]];
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

    // Call the helper function to get the near and far points
    vert.near = unproject_point(p.x, p.y, -1.0, aInvProjectionView);
    vert.far  = unproject_point(p.x, p.y, 1.0, aInvProjectionView);
    
    vert.position = float4(p, 0.0, 1.0);

    return vert;
}