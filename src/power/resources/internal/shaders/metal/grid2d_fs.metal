#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
};

fragment float4 fragment_main(VertexOut in [[stage_in]],
                              constant float2 &scrollOffset [[buffer(0)]],
                              constant float &gridSize [[buffer(1)]],
                              constant float &lineWidth [[buffer(2)]]) {

    // Adjust the coordinates by the scroll offset to create a moving grid
    float2 gridCoord = (in.texCoord + scrollOffset) / gridSize;

    // Create the grid lines by checking how close the current fragment is to the nearest grid line
    float lineX = abs(fract(gridCoord.x) - 0.5) * gridSize;
    float lineY = abs(fract(gridCoord.y) - 0.5) * gridSize;

    // Define line color and background color
    float lineIntensity = step(lineWidth, lineX) * step(lineWidth, lineY);
    float4 lineColor = float4(0.0, 0.0, 0.0, 1.0); // Black lines
    float4 backgroundColor = float4(1.0, 1.0, 1.0, 1.0); // White background

    return mix(lineColor, backgroundColor, lineIntensity);
}
