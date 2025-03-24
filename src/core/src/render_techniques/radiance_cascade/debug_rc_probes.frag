Texture2D g_CascadeTex;
SamplerState g_nearestSampler;
uint2 g_buffer_dimensions;

float4 main(in float4 pos : SV_Position) : SV_Target
{
    uint2 target_dims = g_buffer_dimensions;
    //uint3 source_dims;
    //g_CascadeTex.GetDimensions(0, source_dims.x, source_dims.y, source_dims.z);
    float2 uv = (pos.xy + 0.5) / target_dims.xy;
    float4 tex = g_CascadeTex.Sample(g_nearestSampler, uv);
	//float4 tex = g_CascadeTex.Load(int3(pos.xy, 0));
	return tex;
}
