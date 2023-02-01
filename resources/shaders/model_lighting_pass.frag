#version 460 core

in vec2 vOut_texcoord;

layout (binding = 0) uniform sampler2D gbufferDiffuse;
layout (binding = 1) uniform sampler2D gbufferPosition;
layout (binding = 2) uniform sampler2D gbufferNormal;

layout (location = 0) out vec4 pOut_color;

uniform mat4 u_view;

uniform vec3 u_lightDir;
uniform vec3 u_lightColor;
uniform float u_lightIntensityAmbient;
uniform float u_lightIntensitySpecular;

void main()
{
    vec4 gbufferDiffuseSample = texture(gbufferDiffuse, vOut_texcoord);
    vec4 gbufferPositionSample = texture(gbufferPosition, vOut_texcoord);
    vec4 gbufferNormalSample = texture(gbufferNormal, vOut_texcoord);

    vec3 viewLightDir = normalize(mat3(u_view) * u_lightDir);
    vec3 viewPosition = gbufferPositionSample.xyz;
    vec3 viewNormal = normalize(gbufferNormalSample.xyz);
    vec3 surfaceColor = gbufferDiffuseSample.rgb;
    float specularColor = gbufferDiffuseSample.a;
    float specularExponent = gbufferNormalSample.a;

    vec3 ambient = u_lightIntensityAmbient * u_lightColor;

    vec3 diffuse = max(0, dot(viewNormal, viewLightDir)) * u_lightColor;

    vec3 viewDir = normalize(-viewPosition);
    vec3 viewLightDirR = reflect(viewLightDir, viewNormal);
    float specularFactor = pow(max(0, dot(viewDir, viewLightDir)), specularExponent) * specularColor;
    vec3 specular = u_lightIntensitySpecular * specularFactor * u_lightColor;

    vec3 outColor = (ambient + diffuse + specular) * surfaceColor;

    pOut_color = vec4(outColor, 1);
}
