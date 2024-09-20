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

struct Bone {
    float4x4 transform;
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
                             constant Bone *bones [[buffer(12)]],
                             uint id [[vertex_id]]) {
    const int MAX_BONES = 128;
    const int MAX_BONE_INFLUENCE = 4;

    VertexOut vert;

    float4 pos = float4(aPosition[id], 1.0);
    float3 norm = aNormal[id];
    float4 totalPosition = float4(0.0);
    float3 totalNormal = float3(0.0);

    float maxWeight = 0.0;
    int maxBoneId = aBoneIds[id][0];

    for(int i = 0; i < MAX_BONE_INFLUENCE; i++) {
        if(aBoneIds[id][i] == -1) 
            continue;
        if(aBoneIds[id][i] >= MAX_BONES) {
            totalPosition = pos;
            totalNormal = norm;
            break;
        }

        float4 localPosition = bones[aBoneIds[id][i]].transform * pos;

        // Manually extract the 3x3 matrix from the 4x4 bone matrix
        float3x3 bonemat3 = float3x3(
            bones[aBoneIds[id][i]].transform[0].xyz,
            bones[aBoneIds[id][i]].transform[1].xyz,
            bones[aBoneIds[id][i]].transform[2].xyz
        );

        float3 localNormal = bonemat3 * norm;
        totalNormal += localNormal * aWeights[id][i];
        totalPosition += localPosition * aWeights[id][i];
        
        if(maxWeight < aWeights[id][i]) {
            maxWeight = aWeights[id][i];
            maxBoneId = aBoneIds[id][i];
        }
    }

    // Extract the 3x3 matrix from the model matrix
    float3x3 model3x3 = float3x3(
        aModel[0].xyz,
        aModel[1].xyz,
        aModel[2].xyz
    );

    // Output the transformed position and normal
    vert.Position = aProjection * aView * pos;
    vert.Normal = model3x3 * totalNormal;
    vert.FragPos = float3(aModel * totalPosition);
    vert.TexCoords1 = aTexcoords1[id];
    vert.TexCoords2 = aTexcoords2[id];

    vert.TextureId = aTextureId[id].x; // Access the x component of packed_int2 at index id

    vert.Color = aColor[id];

    return vert;
}
