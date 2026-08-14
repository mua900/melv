// @todo

cbuffer buffer : register(b0, space1) {
    row_major float4x4 ModelViewProjection;
}

struct VSInput {
    float2 position : TEXCOORD0;
    float brightness : TEXCOORD1;
    float4 color : TEXCOORD2;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float brightness : TEXCOORD0;
    float4 color : TEXCOORD1;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.position = mul(ModelViewProjection, float4(input.position, 0.0, 1.0));
    output.brightness = input.brightness;
    output.color = input.color;
    return output;
}
