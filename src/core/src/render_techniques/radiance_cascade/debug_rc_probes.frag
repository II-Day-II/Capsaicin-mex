Texture2D g_CascadeTex;

float4 main(in float4 pos : SV_Position) : SV_Target
{
	float4 tex = g_CascadeTex.Load(int3(pos.xy, 0));
	return tex;
}
