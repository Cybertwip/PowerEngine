#include <metal_stdlib>
using namespace metal;

struct Material {
    float4 ambient;
    float4 diffuse;
    float4 specular;
    float shininess;
    float opacity;
    float diffuse_texture;
};

struct VertexOut {
    float4 Position [[position]];
    float2 TexCoords1;
    float2 TexCoords2;
    float3 Normal;
    float4 Color;
    float3 FragPos;
    int MaterialId;
};

struct FragmentOut {
    float4 color [[color(0)]];  // First attachment: Color
};

fragment FragmentOut fragment_main(VertexOut vert [[stage_in]],
                              constant Material *materials [[buffer(0)]],  
                              constant float4 &color [[buffer(1)]],
                              constant int &identifier [[buffer(2)]],
                              array<texture2d<float, access::sample>, 16> textures,
                              array<sampler, 16> textures_sampler) {
    Material mat = materials[vert.MaterialId];
    texture2d<float> diffuse_texture = textures[vert.MaterialId];
    sampler diffuse_sampler = textures_sampler[vert.MaterialId];

    float4 mat_diffuse;
    if (mat.diffuse_texture != 0.0) {
        // Accessing textures and sampling RGBA (float4)
        float4 tex1 = diffuse_texture.sample(diffuse_sampler, vert.TexCoords1);  // sample rgba
        float4 tex2 = diffuse_texture.sample(diffuse_sampler, vert.TexCoords2);  // sample rgba
        mat_diffuse = (tex1 + tex2) * 0.5;  // Average the two textures
    } else {
        mat_diffuse = float4(mat.diffuse * mat.specular);  // Use material opacity
        mat_diffuse.a = mat.opacity;
    }

    mat_diffuse = mix(mat_diffuse, vert.Color, 0.25);

    float3 final_color = mat_diffuse.rgb;
    float selectionOpacity = mat_diffuse.a;  // Use the alpha from the texture

    if (length(color.rgba) != 2.0) { 
        float brightness = dot(final_color, float3(0.299, 0.587, 0.114));
        float darkness_factor = clamp(1.0 - brightness / 0.05, 0.0, 1.0);
        final_color = mix(final_color, color.rgb, darkness_factor);
        selectionOpacity *= mix(0.3, 1.0, 1.0 - darkness_factor);
        final_color = mix(final_color, color.rgb, 0.25);
    }

    FragmentOut out;

    out.color = float4(final_color, selectionOpacity);

    return out;
}
