// @todo

struct PSInput {
    float4 position : SV_POSITION;
    float brightness : TEXCOORD0;
    float4 color : TEXCOORD1;
};

void main(PSInput input, out float4 fragColor : SV_Target)
{
    fragColor = input.color * input.brightness;
}
