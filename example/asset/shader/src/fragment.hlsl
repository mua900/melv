struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : TEXCOORD1;
};

void main(PSInput input, out float4 fragColor : SV_Target)
{
    fragColor = float4(1, 0, 0, 1);
}
