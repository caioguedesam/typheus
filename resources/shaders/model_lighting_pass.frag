#version 460 core

in vec2 vOut_texcoord;

layout (binding = 0) uniform sampler2D gbufferDiffuse;
layout (binding = 1) uniform sampler2D gbufferPosition;
layout (binding = 2) uniform sampler2D gbufferNormal;

layout (location = 0) out vec4 pOut_color;

uniform mat4 u_view;

void main()
{
    vec4 gbufferDiffuseSample = texture(gbufferDiffuse, vOut_texcoord);
    vec4 gbufferPositionSample = texture(gbufferPosition, vOut_texcoord);
    vec4 gbufferNormalSample = texture(gbufferNormal, vOut_texcoord);

    vec3 dirLight = normalize(vec3(0.5, 1, 0.5));
    vec3 viewDirLight = normalize(mat3(u_view) * dirLight);
    vec3 viewPosition = gbufferPositionSample.xyz;
    vec3 viewNormal = normalize(gbufferNormalSample.xyz);
    vec3 diffuseColor = gbufferDiffuseSample.rgb;
    float specularIntensity = gbufferDiffuseSample.a;
    float specularExponent = gbufferNormalSample.a;

    float ambient = 0.1;
    float diffuse = max(0, dot(viewNormal, viewDirLight));
    vec3 outColor = (ambient + diffuse) * diffuseColor;

    pOut_color = vec4(outColor, 1);
}
