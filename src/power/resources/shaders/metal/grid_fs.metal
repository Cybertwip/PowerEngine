#include <metal_stdlib>
using namespace metal;

struct VertexInput {
    float3 near;
    float3 far;
};

float compute_fade(constant float4x4 &viewMatrix, constant float4x4 &projMatrix, constant float &u_near, constant float &u_far, float3 point) {
    float4 clip_space = projMatrix * viewMatrix * float4(point, 1.0);
    float clip_space_depth = (clip_space.z / clip_space.w) * 2.0 - 1.0;
    float linear_depth = (2.0 * u_near * u_far) / (u_far + u_near - clip_space_depth * (u_far - u_near));
    return linear_depth / u_far;
}

fragment float4 fragment_main(VertexInput in [[stage_in]],
                              constant float &u_near [[buffer(0)]],
                              constant float &u_far [[buffer(1)]],
                              constant float4x4 &aView [[buffer(2)]],
                              constant float4x4 &aProjection [[buffer(3)]]) {
    float t = -in.near.y / (in.far.y - in.near.y);
    float3 R = in.near + t * (in.far - in.near);
    float is_on = step(0.0, t);

    // Helper function to compute grid color
    auto grid = [](float3 point, float scale, bool is_axis) -> float4 {
        float2 coord = point.xz / scale;
        float2 dd = abs(fwidth(coord));
        float2 uv = fract(coord - 0.5) - 0.5;
        float2 grid = abs(uv) / dd;
        float line = min(grid.x, grid.y);
        float min_z = min(dd.y, 1.0);
        float min_x = min(dd.x, 1.0);
        float4 col = float4(1.0);
        col.a = 1.0 - min(line, 1.0);

        if (-1.0 * min_x < point.x && point.x < 0.1 * min_x && is_axis)
            col.rgb = float3(0.427, 0.792, 0.909);
        if (-1.0 * min_z < point.z && point.z < 0.1 * min_z && is_axis)
            col.rgb = float3(0.984, 0.380, 0.490);
        return col;
    };


    float fade = smoothstep(0.32, 0.0, compute_fade(aView, aProjection, u_near, u_far, R));
    float4 o_color = grid(R, 32.0, true);
    o_color *= fade * is_on;

    return o_color;
}
