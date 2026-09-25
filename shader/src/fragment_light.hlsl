struct PSInput {
    float4 position : SV_POSITION;
    float radius : TEXCOORD0;
    float brightness : TEXCOORD1;
    float4 color : TEXCOORD2;
};

void main(PSInput input, out float4 fragColor : SV_Target)
{
    fragColor = float4(1,1,1,1);//input.color * input.brightness;
}
