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
    int TextureId;
    int VertexId;
};

struct FragmentOut {
    float4 color [[color(0)]];  // First attachment: Color
    int entityId [[color(1)]];  // Second attachment: Entity ID
};

fragment FragmentOut fragment_main(VertexOut vert [[stage_in]],
                              constant Material *materials [[buffer(0)]], 
                              constant float4 &color [[buffer(1)]],
                              constant int &identifier [[buffer(2)]],
                              constant float4x4 &aView [[buffer(3)]],
                              array<texture2d<float, access::sample>, 4> textures,
                              array<sampler, 4> textures_sampler) {
    Material mat = materials[vert.TextureId];

    texture2d<float> diffuse_texture = textures[vert.TextureId];
    sampler diffuse_sampler = textures_sampler[vert.TextureId];

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

    mat_diffuse = mix(mat_diffuse, vert.Color, 0.5);

        
    int entityId = identifier;

    if (mat.diffuse.r != 0.0) {
        entityId = -1;
    } else if (mat.diffuse.b != 0.0) {
        entityId = -2;
    }  else if (mat.diffuse.g != 0.0) {
        entityId = -3;
    }

    FragmentOut out;

    out.color = mat_diffuse;
    out.entityId = entityId;

    return out;
}
