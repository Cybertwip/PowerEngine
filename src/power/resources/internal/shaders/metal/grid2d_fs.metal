#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
};

fragment float4 fragment_main(VertexOut in [[stage_in]],
                              constant float2 &scrollOffset [[buffer(0)]],
                              constant float &gridSize [[buffer(1)]],
                              constant float &lineWidth [[buffer(2)]]) {
    // Use the position on screen as the grid coordinate
    float2 coord = (in.position.xy / gridSize) + scrollOffset;

    // Calculate proximity to the nearest grid line
    float lineX = abs(fract(coord.x) - 0.5) * gridSize;
    float lineY = abs(fract(coord.y) - 0.5) * gridSize;

    // Create grid lines based on line thickness
    float lineIntensity = step(lineWidth, lineX) * step(lineWidth, lineY);

    // Blend between grid color and background
    float4 gridColor = float4(0.0, 0.0, 0.0, 1.0); // Black lines
    float4 backgroundColor = float4(1.0, 1.0, 1.0, 1.0); // White background
    return mix(gridColor, backgroundColor, lineIntensity);
}
