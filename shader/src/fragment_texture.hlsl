Texture2D colorTexture : register(t0, space2);
SamplerState colorSampler : register(s0, space2);

struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : TEXCOORD1;
};

void main(PSInput input, out float4 fragColor : SV_Target)
{
    float4 sampleColor = colorTexture.Sample(colorSampler, input.uv);
    fragColor = input.color * sampleColor;
}
