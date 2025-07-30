#include <metal_stdlib>
using namespace metal;

// --- Fragment Shader ---
// Final output from the fragment shader.

struct VertexIO {
    float4 position [[position]];
    float3 near;
    float3 far;
};

struct FragmentOutput {
    float4 color [[color(0)]];
    float depth [[depth(any)]];
};



// Helper function to compute the visual appearance of the grid lines.
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

    // Highlight the Z-axis
    if ((-1.0 * min_x) < point.x && point.x < (0.1 * min_x) && is_axis) {
        col.rgb = float3(0.427, 0.792, 0.909); // Blue
    }
    // Highlight the X-axis
    if ((-1.0 * min_z) < point.z && point.z < (0.1 * min_z) && is_axis) {
        col.rgb = float3(0.984, 0.380, 0.490); // Red
    }
    return col;
}


// The fragment shader's main function. It runs for every pixel covered by the quad.
fragment FragmentOutput fragment_main(VertexIO in [[stage_in]],
                                     constant float &u_near [[buffer(0)]],
                                     constant float &u_far [[buffer(1)]],
                                     constant float4x4 &aView [[buffer(2)]]) {
    // The view ray's direction vector.
    float3 ray_direction = in.far - in.near;

    // The denominator for the ray-plane intersection calculation.
    // This is the Y-component of the ray's direction.
    float den = ray_direction.y;

    // --- Stability Check at Horizon ---
    // If the ray is nearly parallel to the grid plane (den is close to zero),
    // discard the fragment. This happens at the horizon. This check is necessary
    // to prevent division by zero, but it is also the source of the flickering,
    // as camera movement can cause `den` to hover around the threshold.
    if (abs(den) < 1e-6) {
        discard_fragment();
    }

    // Calculate the intersection of the view ray with the grid plane (y=0).
    // 't' is the parameter along the ray where the intersection occurs.
    float t = -in.near.y / den;

    // If t is negative, the intersection point is "behind" the camera.
    if (t < 0.0) {
        discard_fragment();
    }
    
    // Calculate the world-space intersection point R.
    float3 R = in.near + t * ray_direction;

    // --- Clipping and Depth Calculation ---
    // Transform the world point R to view space to get its distance from the camera.
    float4 view_position = aView * float4(R, 1.0);

    // In view space, distance along the view direction is the negative Z coordinate.
    float view_distance = -view_position.z;

    // Discard fragments that are outside the near/far clipping planes.
    // The check against u_far provides the main clipping at the horizon.
    if (view_distance > u_far || view_distance < u_near) {
        discard_fragment();
    }

    // --- Final Color and Depth Output ---
    // Compute the grid pattern color based on the world position.
    float4 o_color = compute_grid(R, 32.0, true);

    FragmentOutput out;
    out.color = o_color;
    
    // The depth buffer requires a value in the [0, 1] range.
    // We calculate this directly from the view_distance we already have.
    out.depth = (view_distance - u_near) / (u_far - u_near);

    return out;
}