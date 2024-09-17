#include <metal_stdlib>
using namespace metal;

// Fragment Shader
struct FragmentOut {
    float4 color [[color(0)]];   // First attachment: Color
    int entityId [[color(1)]];  // Second attachment: Entity ID
};

fragment FragmentOut fragment_main(constant float4 &u_color [[buffer(0)]],
                                   constant int &identifier [[buffer(1)]]) {

    FragmentOut out;

    out.color = u_color;
    out.entityId = identifier;

    return out;
}
