#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 Position [[position]];
    float2 TexCoords1;
    float2 TexCoords2;
    int MaterialId;
};

struct Bone {
    float4x4 transform;
};

vertex VertexOut vertex_main(
    const device packed_float3 *const aPosition [[buffer(0)]],
    const device packed_float2 *const aTexcoords1 [[buffer(3)]],
    const device packed_float2 *const aTexcoords2 [[buffer(4)]],
    const device packed_int4 *const aBoneIds [[buffer(5)]],
    const device packed_float4 *const aWeights [[buffer(6)]],
    const device int *const aMaterialId [[buffer(7)]],
    constant float4x4 &aProjection [[buffer(8)]],
    constant float4x4 &aView [[buffer(9)]],
    constant float4x4 &aModel [[buffer(10)]],
    constant Bone *bones [[buffer(11)]],
    uint id [[vertex_id]]
) {
    const int MAX_BONE_INFLUENCE = 4;

    VertexOut vert;

    float4 pos = float4(aPosition[id], 1.0);

    float4 skinnedPosition = float4(0.0);

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
    }


    float4 worldPosition = aModel * skinnedPosition;
    vert.Position = aProjection * aView * worldPosition;

    vert.TexCoords1 = aTexcoords1[id];
    vert.TexCoords2 = aTexcoords2[id];

    vert.MaterialId = MaterialId[id];

    return vert;
}
