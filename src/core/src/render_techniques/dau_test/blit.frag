
Texture2D ColorBuffer;
float4 main(in float4 pos : SV_POSITION, float2 uv : TEXCOORD) : SV_target
{
	int2 dims;
	ColorBuffer.GetDimensions(dims.x, dims.y);
	return ColorBuffer.Load(int3(uv * dims, 0));
}
