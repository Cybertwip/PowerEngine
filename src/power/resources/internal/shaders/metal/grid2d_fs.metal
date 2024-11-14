#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
};

fragment float4 fragment_main(VertexOut in [[stage_in]],
                              constant float &gridSize [[buffer(0)]],
                              constant float &lineWidth [[buffer(1)]]) {
    float2 coord = in.texCoord * gridSize;
    float lineX = abs(fract(coord.x) - 0.5);
    float lineY = abs(fract(coord.y) - 0.5);
    
    // Calculate line intensity based on proximity to grid lines
    float lineIntensity = smoothstep(0.5 - lineWidth, 0.5, lineX) * 
                          smoothstep(0.5 - lineWidth, 0.5, lineY);

    // Invert the line intensity to create dark lines on light background
    return float4(1.0 - lineIntensity, 1.0 - lineIntensity, 1.0 - lineIntensity, 1.0);
}
