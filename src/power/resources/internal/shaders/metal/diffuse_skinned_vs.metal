#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 Position [[position]];
    float2 TexCoords1;
    float2 TexCoords2;
    float3 Normal;
    float4 Color;
    float3 FragPos;
    int MaterialId;
};

struct Bone {
    float4x4 transform;
};

struct Instance {
    float3 Position;
    float3 Normal;
    float4 Color;
    float2 Texcoords1;
    float2 Texcoords2;
};

vertex VertexOut vertex_main(
    constant Instance *aInstances [[buffer(0)]],
    const device int *const aInstanceId [[buffer(1)]],
    const device int *const aMaterialId [[buffer(2)]],    
    const device packed_int4 *const aBoneIds [[buffer(3)]],
    const device packed_float4 *const aWeights [[buffer(4)]],
    constant float4x4 &aProjection [[buffer(5)]],
    constant float4x4 &aView [[buffer(6)]],
    constant float4x4 &aModel [[buffer(7)]],
    constant Bone *bones [[buffer(8)]],
    uint id [[vertex_id]]
) {
    const int MAX_BONE_INFLUENCE = 4;

    int instanceIndex = aInstanceId[id];
    Instance inst = aInstances[instanceIndex];

    VertexOut vert;

    float4 pos = float4(inst.Position, 1.0);
    float3 norm = inst.Normal;

    float4 skinnedPosition = float4(0.0);
    float3 skinnedNormal = float3(0.0);

    // Loop through the bone influences
    for(int i = 0; i < MAX_BONE_INFLUENCE; i++) {
        int boneId = aBoneIds[id][i]; // Correct indexing for bone IDs
        if (boneId == -1)
            continue;
            
        float weight = aWeights[id][i]; // Correct indexing for bone weights
       
        if(boneId < 0 || weight == 0.0) 
            continue;

        float4x4 boneTransform = bones[boneId].transform;

        skinnedPosition += weight * (boneTransform * pos);
        skinnedNormal += weight * ((boneTransform * float4(norm, 0.0)).xyz);
    }

    // Normalize the accumulated normal
    skinnedNormal = normalize(skinnedNormal);

    // Transform the position and normal to clip space
    float4 worldPosition = aModel * skinnedPosition;
    vert.Position = aProjection * aView * worldPosition;

    // Transform the normal to world space
    float3 worldNormal = normalize((aModel * float4(skinnedNormal, 0.0)).xyz);
    vert.Normal = worldNormal;

    // Position in world space for fragment shader calculations
    vert.FragPos = worldPosition.xyz;

    vert.TexCoords1 = inst.Texcoords1;
    vert.TexCoords2 = inst.Texcoords2;

    vert.MaterialId = aMaterialId[id];
    vert.Color = inst.Color;

    return vert;
}
