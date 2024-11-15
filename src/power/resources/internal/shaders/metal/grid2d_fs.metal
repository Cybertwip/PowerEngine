#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float4 scroll;
};

fragment float4 fragment_main(VertexOut in [[stage_in]],
                              constant float &gridSize [[buffer(1)]],
                              constant float &lineWidth [[buffer(2)]]) {
    // Adjusted grid coordinate calculation
    float2 coord = (in.position.xy + in.scroll.xy) / gridSize;

    // Calculate proximity to the nearest grid line with line thickness
    float lineX = abs(fract(coord.x) - 0.5) * gridSize;
    float lineY = abs(fract(coord.y) - 0.5) * gridSize;
    float lineIntensity = step(lineWidth, lineX) * step(lineWidth, lineY);

    // Colors with solid background
    float4 gridColor = float4(0.0, 0.0, 0.0, 1.0); // Black lines
    float4 backgroundColor = float4(1.0, 1.0, 1.0, 0.0); // White solid background
    return mix(gridColor, backgroundColor, lineIntensity);
}
