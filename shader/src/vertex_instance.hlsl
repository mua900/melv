cbuffer buffer : register(b0, space1) {
    row_major float4x4 ModelViewProjection;
}

struct VSInput {
    float2 position : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float3 instance_position : TEXCOORD2;
    float rotation : TEXCOORD3;
    float2 scale : TEXCOORD4;
    float4 color : TEXCOORD5;
    float2 sourceOffset : TEXCOORD6;
    float2 sourceScale : TEXCOORD7;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : TEXCOORD1;
};

VSOutput main(VSInput input)
{
    float2 uv = input.uv * input.sourceScale + input.sourceOffset;
    float2 scale = input.scale * 256;

    VSOutput output;
    float2 p = input.position;
    p.x *= scale.x;
    p.y *= scale.y;
    float c = cos(input.rotation);
    float s = sin(input.rotation);
    float2 pos = input.instance_position.xy + float2(p.x * c - p.y * s, p.x * s + p.y * c);

    output.position = mul(ModelViewProjection, float4(pos, input.instance_position.z, 1.0));
    output.uv = uv;
    output.color = input.color;
    return output;
}
