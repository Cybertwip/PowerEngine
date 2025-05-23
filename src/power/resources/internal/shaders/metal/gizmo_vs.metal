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

vertex VertexOut vertex_main(const device packed_float3 *const aPosition [[buffer(0)]],
                             const device packed_float3 *const aNormal [[buffer(1)]],
                             const device packed_float4 *const aColor [[buffer(2)]],
                             const device packed_float2 *const aTexcoords1 [[buffer(3)]],
                             const device packed_float2 *const aTexcoords2 [[buffer(4)]],
                             const device int *const aMaterialId [[buffer(5)]],
                             constant float4x4 &aProjection [[buffer(6)]],
                             constant float4x4 &aView [[buffer(7)]],
                             constant float4x4 &aModel [[buffer(8)]],
                             uint id [[vertex_id]]) {
    VertexOut vert;

    // Transform the vertex position
    float4 worldPosition = aModel * float4(aPosition[id], 1.0);
    vert.Position = aProjection * aView * worldPosition;
    
    // Extract the upper-left 3x3 submatrix from the model matrix for normal transformation
    float3x3 normalMatrix = float3x3(float3(aModel[0].xyz), float3(aModel[1].xyz), float3(aModel[2].xyz));
    // Cast packed_float3 to float3 for the matrix multiplication
    vert.Normal = normalMatrix * float3(aNormal[id]);
    
    vert.FragPos = worldPosition.xyz;
    vert.TexCoords1 = aTexcoords1[id];
    vert.TexCoords2 = aTexcoords2[id];
    
    vert.MaterialId = aMaterialId[id]; // Access the x component of packed_int2 at index id

    vert.Color = aColor[id];
    
    return vert;
}
