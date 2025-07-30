#include <metal_stdlib>
using namespace metal;

// --- Data Structures ---

// Output from the vertex shader, input to the fragment shader.
// It defines a view ray for each fragment by providing its start (near)
// and end (far) points in world space.
struct VertexIO {
    float4 position [[position]];
    float3 near;
    float3 far;
};

// --- Vertex Shader ---

// Helper function to unproject a 2D screen point into a 3D world space point.
// This is used to find the start and end points of the view ray.
float3 unproject_point(float x, float y, float z, constant float4x4 &invProjView) {
    float4 unproj_point = invProjView * float4(x, y, z, 1.0);
    return unproj_point.xyz / unproj_point.w;
}

// The vertex shader's main function. It runs for each vertex of a full-screen quad
// and calculates the world-space view ray for that vertex.
vertex VertexIO vertex_main(const device packed_float2 *const aPosition [[buffer(0)]],
                           constant float4x4 &aInvProjectionView [[buffer(1)]],
                           uint id [[vertex_id]]) {
    VertexIO out;
    
    // Get the 2D position of the current vertex from the input buffer.
    float2 p = aPosition[id];

    // Calculate the world-space positions on the near and far clipping planes
    // that correspond to this vertex. These two points define the view ray.
    // NDC depth values (-1 for near, 1 for far) are used.
    out.near = unproject_point(p.x, p.y, -1.0, aInvProjectionView);
    out.far  = unproject_point(p.x, p.y, 1.0, aInvProjectionView);
    
    // Pass the vertex position through unmodified for the full-screen quad.
    out.position = float4(p, 0.0, 1.0);

    return out;
}
