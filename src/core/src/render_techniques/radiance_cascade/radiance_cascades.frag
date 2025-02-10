
#include "../../gpu_shared.h"


Texture2D g_DepthBuffer;
Texture2D g_ShadingNormalBuffer;
Texture2D g_VisibilityBuffer;


Texture2D g_LutBuffer;
uint g_LutSize;

StructuredBuffer<uint> g_IndexBuffer;
StructuredBuffer<Vertex> g_VertexBuffer;
StructuredBuffer<Mesh> g_MeshBuffer;
StructuredBuffer<Instance> g_InstanceBuffer;
StructuredBuffer<Material> g_MaterialBuffer;

Texture2D<float4> g_IrradianceBuffer;
// RWTexture2D<float4> g_ReflectionBuffer; // reflections disabled for now

Texture2D g_TextureMaps[] : register(space99);
SamplerState g_NearestSampler;
SamplerState g_LinearSampler;
SamplerState g_TextureSampler;

#include "../../geometry/geometry.hlsl"
#include "../../geometry/mesh.hlsl"
#include "../../materials/material_evaluation.hlsl"
#include "../../materials/material_sampling.hlsl"
#include "../../math/transform.hlsl"

uint2 g_BufferDimensions;
float4x4 g_ViewProjectionInverse;
float3 g_Eye;

float3 InverseProject(in float4x4 transform, in float2 uv, in float depth)
{
    return transformPointProjection(float3(2.0f * float2(uv.x, 1.0f - uv.y) - 1.0f, depth), transform);
}

struct PS_OUTPUT
{
	float4 lighting : SV_Target0;
};

PS_OUTPUT ResolveRCGI(in float4 pos : SV_Position)
{
    uint2 did = uint2(pos.xy);
    
    float depth = g_DepthBuffer.Load(int3(did, 0)).x;
    float3 normal = normalize(2.0f * g_ShadingNormalBuffer.Load(int3(did, 0)).xyz - 1.0f);

    if (depth >= 1.0f)
    {
        PS_OUTPUT output;
        output.lighting = float4(0.0f, 0.0f, 0.0f, 1.0f);
        return output;
    }

    float4 visibility = g_VisibilityBuffer.Load(int3(did, 0));
    uint instanceID = asuint(visibility.z);
    uint primitiveID = asuint(visibility.w);

    Instance instance = g_InstanceBuffer[instanceID];
    Mesh mesh = g_MeshBuffer[instance.mesh_index];

    // Get UV values from buffers
    UVs uvs = fetchUVs(mesh, primitiveID);

    float2 uv = (did + 0.5f) / g_BufferDimensions;
    float3 world = InverseProject(g_ViewProjectionInverse, uv, depth);
    float3 view_direction = normalize(g_Eye - world);
    float2 mesh_uv = interpolate(uvs.uv0, uvs.uv1, uvs.uv2, visibility.xy);
    float dotNV = saturate(dot(normal, view_direction));

    Material material = g_MaterialBuffer[instance.material_index];
    MaterialEvaluated material_evaluated = MakeMaterialEvaluated(material, mesh_uv);
    MaterialEmissive emissiveMaterial = MakeMaterialEmissive(material, mesh_uv);

    MaterialBRDF materialBRDF = MakeMaterialBRDF(material_evaluated);
    // TODO: reenable disable specular materials, see gi10.frag

    PS_OUTPUT output;

    // compute diffuse compensation term with specular dominant half-vector
    float3 specular_dominant_direction = calculateGGXSpecularDirection(normal, view_direction, sqrt(materialBRDF.roughnessAlpha));
    float3 specular_dominant_half_vector = normalize(view_direction + specular_dominant_direction);
    float dotHV = saturate(dot(view_direction, specular_dominant_half_vector));
    float3 diffuse_compensation = diffuseCompensation(materialBRDF.F0, dotHV);

    // diffuse term
    //float3 irradiance = g_IrradianceBuffer[did].xyz;
    float3 irradiance = g_IrradianceBuffer.Sample(g_LinearSampler, uv).xyz;
    float3 diffuse = evaluateLambert(materialBRDF.albedo) * diffuse_compensation * irradiance;

    // compute specular term with split-sum approximation
    //float4 radiance_sum = (material_evaluated.roughness > g_GlossyReflectionsConstants.high_roughness_threshold ? float4(irradiance, PI) : g_ReflectionBuffer[did]); // fall back to filtered irradiance past threshold
    float4 radiance_sum = float4(irradiance, PI);
    float2 lut = g_LutBuffer.SampleLevel(g_LinearSampler, float2(dotNV, material_evaluated.roughness), 0.0f).xy;
    float3 directional_albedo = saturate(materialBRDF.F0 * lut.x + (1.0f - materialBRDF.F0) * lut.y);
    float3 specular = directional_albedo * (radiance_sum.xyz / max(radiance_sum.w, 1.0f));
    output.lighting = float4(emissiveMaterial.emissive + diffuse + specular, 1.0f);


    return output;
}
