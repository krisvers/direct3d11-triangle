struct VS_INPUT {
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 color : COLOR;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 color : COLOR;
};

cbuffer cbuffer0 : register(b0) {
    float4x4 uTransform;
}

sampler sample0 : register(s0);
Texture2D<float4> texture0 : register(t0);

PS_INPUT vs(VS_INPUT input) {
    PS_INPUT output;
    output.pos = mul(uTransform, float4(input.pos, 1));
    output.uv = input.uv;
    output.color = input.color;
    return output;
}

float4 ps(PS_INPUT input) : SV_TARGET {
    float4 tex = texture0.Sample(sample0, input.uv);
    return float4(input.color, 1);
}