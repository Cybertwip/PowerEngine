#include <metal_stdlib>
using namespace metal;

struct FragmentIn {
    float3 fragPos;
    float3 normal;
};

struct Light {
    float3 position;
    float3 color;
};

struct Material {
    float3 color;
};

fragment float4 fragment_main(FragmentIn in [[stage_in]], constant Light& light [[buffer(0)]], constant Material& material [[buffer(1)]]) {
    float ambientStrength = 0.1;
    float3 ambient = ambientStrength * light.color;

    float3 norm = normalize(in.normal);
    float3 lightDir = normalize(light.position - in.fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    float3 diffuse = diff * light.color;

    float3 result = (ambient + diffuse) * material.color;
    return float4(result, 1.0);
}
