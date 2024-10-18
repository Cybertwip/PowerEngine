#include <metal_stdlib>
using namespace metal;

struct VertexInput {
    float3 near;
    float3 far;
};

struct FragmentOutput {
    float4 color [[color(0)]];
    float depth [[depth(any)]];
};

// Helper function to compute grid color
float4 compute_grid(float3 point, float scale, bool is_axis) {
    float2 coord = point.xz / scale;
    float2 dd = abs(fwidth(coord));
    float2 uv = fract(coord - 0.5) - 0.5;
    float2 grid = abs(uv) / dd;
    float line = min(grid.x, grid.y);
    float min_z = min(dd.y, 1.0);
    float min_x = min(dd.x, 1.0);
    float4 col = float4(1.0);
    col.a = 1.0 - min(line, 1.0);

    if ((-1.0 * min_x) < point.x && point.x < (0.1 * min_x) && is_axis) {
        col.rgb = float3(0.427, 0.792, 0.909);
    }
    if ((-1.0 * min_z) < point.z && point.z < (0.1 * min_z) && is_axis) {
        col.rgb = float3(0.984, 0.380, 0.490);
    }
    return col;
}

// Fade computation modified for stronger fade near the far plane
float compute_fade(constant float4x4 &viewMatrix, constant float4x4 &projMatrix, constant float &u_near, constant float &u_far, float3 point) {
    float4 clip_space = projMatrix * viewMatrix * float4(point, 1.0);
    float clip_space_depth = (clip_space.z / clip_space.w) * 2.0 - 1.0;
    float near_val = u_near; 
    float far_val = u_far;
    float depth = (clip_space_depth - near_val) / (far_val - near_val);
    return depth / u_far;
}

// Depth computation corrected to map depth to [0, 1]
float compute_depth(float3 point, constant float4x4 &aView, constant float4x4 &aProjection, constant float &u_near, constant float &u_far) {
    float4 clip_space = aProjection * aView * float4(point, 1.0);
    float clip_space_depth = clip_space.z / clip_space.w;
    
    // Map from clip space depth [-1, 1] to view space depth [near, far]
    float depth_view = ((clip_space_depth + 1.0) / 2.0) * (u_far - u_near) + u_near;
    
    // Normalize to [0, 1] for depth buffer
    float normalized_depth = (depth_view - u_near) / (u_far - u_near);
    
    return normalized_depth;
}

fragment FragmentOutput fragment_main(VertexInput in [[stage_in]],
                                      constant float &u_near [[buffer(0)]],
                                      constant float &u_far [[buffer(1)]],
                                      constant float4x4 &aView [[buffer(2)]],
                                      constant float4x4 &aProjection [[buffer(3)]]) {
    float t = -in.near.y / (in.far.y - in.near.y);
    float3 R = in.near + t * (in.far - in.near);
    float is_on = step(0.0, t);

    // Compute grid color using the separate function
    float4 o_color = compute_grid(R, 32.0, true);
    
    // Adjusted fade range for stronger fade-out near the far plane
    float fade = smoothstep(0.25, 0.0, compute_fade(aView, aProjection, u_near, u_far, R));
    o_color *= fade * is_on;

    FragmentOutput out;

    out.color = o_color;
    out.depth = compute_depth(R, aView, aProjection, u_near, u_far);

    return out;
}