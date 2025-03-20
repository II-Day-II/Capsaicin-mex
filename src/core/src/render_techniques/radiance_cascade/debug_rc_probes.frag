Texture2D g_CascadeTex;
SamplerState g_nearestSampler;

float4 main(in float4 pos : SV_Position) : SV_Target
{
    uint3 dims = uint3(1920,1080,0);
	//g_CascadeTex.GetDimensions(0, dims.x, dims.y, dims.z);
    float2 uv = (pos.xy + 0.5) / dims.xy;
    float4 tex = g_CascadeTex.Sample(g_nearestSampler, uv);
	//float4 tex = g_CascadeTex.Load(int3(pos.xy, 0));
	return tex;
}
