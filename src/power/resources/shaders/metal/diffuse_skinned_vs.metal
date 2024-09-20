#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 Position [[position]];
    float2 TexCoords1;
    float2 TexCoords2;
    float3 Normal;
    float4 Color;
    float3 FragPos;
    int TextureId;
};

vertex VertexOut vertex_main(const device packed_float3 *const aPosition [[buffer(0)]],
                             const device packed_float3 *const aNormal [[buffer(1)]],
                             const device packed_float4 *const aColor [[buffer(2)]],
                             const device packed_float2 *const aTexcoords1 [[buffer(3)]],
                             const device packed_float2 *const aTexcoords2 [[buffer(4)]],
                             const device packed_int4 *const aBoneIds [[buffer(5)]],
                             const device packed_float4 *const aWeights [[buffer(6)]],
                             const device packed_int2 *const aTextureId [[buffer(7)]],
                             constant float4x4 &aProjection [[buffer(9)]],
                             constant float4x4 &aView [[buffer(10)]],
                             constant float4x4 &aModel [[buffer(11)]],
                             const device float4x4 *const bones [[buffer(12)]],
                             uint id [[vertex_id]]) {
    VertexOut vert;
    
    // Initialize position and normal with zeros for skinning
    float4 skinnedPosition = float4(0.0);
    float3 skinnedNormal = float3(0.0);

    // Perform skinning by blending bone transformations
    for (int i = 0; i < 4; ++i) {
        int boneId = aBoneIds[id][i];
        float weight = aWeights[id][i];
        float4x4 boneTransform = bones[boneId];

        // Transform the position and normal
        skinnedPosition += (boneTransform * float4(aPosition[id], 1.0)) * weight;

        // Extract the upper-left 3x3 submatrix from the bone transformation matrix for normal transformation
        float3x3 boneNormalMatrix = float3x3(float3(boneTransform[0].xyz), 
                                             float3(boneTransform[1].xyz), 
                                             float3(boneTransform[2].xyz));
        skinnedNormal += (boneNormalMatrix * float3(aNormal[id])) * weight;
    }

    // Output the transformed position and normal
    vert.Position = aProjection * aView * skinnedPosition;
    vert.Normal = normalize(skinnedNormal);
    
    vert.FragPos = skinnedPosition.xyz;
    vert.TexCoords1 = aTexcoords1[id];
    vert.TexCoords2 = aTexcoords2[id];
    
    vert.TextureId = aTextureId[id].x; // Access the x component of packed_int2 at index id

    vert.Color = aColor[id];
    
    return vert;
}
