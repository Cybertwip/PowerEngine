#include <metal_stdlib>
using namespace metal;

// Input from the vertex shader, representing points on the near and far planes.
struct VertexInput {
    float3 near;
    float3 far;
};

// Output from the fragment shader.
struct FragmentOutput {
    float4 color [[color(0)]];
    float depth [[depth(any)]];
};

// Helper function to compute the visual appearance of the grid lines.
// It draws lines on the XZ plane and highlights the X and Z axes.
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

// Computes the final depth value for the fragment in the [0, 1] range.
// This is used by the GPU for depth testing to ensure objects occlude each other correctly.
float compute_depth(float3 point, constant float4x4 &aView, constant float4x4 &aProjection, constant float &u_near, constant float &u_far) {
    float4 clip_space = aProjection * aView * float4(point, 1.0);
    float clip_space_depth = clip_space.z / clip_space.w;
    
    // This re-linearization is specific to how the projection was set up.
    // It maps normalized device coordinates back to view-space depth.
    float depth_view = ((clip_space_depth * 2.0 - 1.0) + 1.0) / 2.0 * (u_far - u_near) + u_near;
    
    // Normalize to [0, 1] for the depth buffer.
    float normalized_depth = (depth_view - u_near) / (u_far - u_near);
    
    return normalized_depth;
}

// Main function for the fragment shader.
// It calculates the color for each pixel of the grid plane.
fragment FragmentOutput fragment_main(VertexInput in [[stage_in]],
                                     constant float &u_near [[buffer(0)]],
                                     constant float &u_far [[buffer(1)]],
                                     constant float4x4 &aView [[buffer(2)]],
                                     constant float4x4 &aProjection [[buffer(3)]]) {
    // Perform a ray-plane intersection to find the point 'R' on the grid (y=0).
    // The ray is defined by the near and far points from the vertex shader.
    float t = -in.near.y / (in.far.y - in.near.y);
    float3 R = in.near + t * (in.far - in.near);

    // If 't' is negative, the intersection is behind the camera, so we discard the pixel.
    // A very small denominator means the view ray is almost parallel to the grid,
    // representing the horizon. We discard these fragments to avoid floating point issues.
    if (t < 0.0 || abs(in.far.y - in.near.y) < 1e-6) {
        discard_fragment();
    }

    // --- Horizon Clipping Logic ---
    // To correctly clip the grid at the horizon (far plane), we must check the
    // distance of the point from the camera in view space.
    float4 view_position = aView * float4(R, 1.0);

    // In view space, the camera is at the origin looking down the negative Z-axis.
    // Therefore, the distance from the camera along the view direction is -view_position.z.
    float view_distance = -view_position.z;

    // Discard any fragment that is beyond the far clipping plane.
    // This creates a hard clip at the far plane, which appears as a straight line at the horizon.
    if (view_distance > u_far) {
        discard_fragment();
    }

    // --- Final Color and Depth Calculation ---
    // Compute the base color for the grid lines.
    float4 o_color = compute_grid(R, 32.0, true);

    // Prepare the final output for this fragment.
    FragmentOutput out;
    out.color = o_color;
    // The depth value must also be computed correctly for the GPU's depth test.
    out.depth = compute_depth(R, aView, aProjection, u_near, u_far);

    return out;
}
