#version 460 core

#define MAX_POINT_LIGHTS 16
#define AMBIENT_LIGHT_INTENSITY     0.1
#define SPECULAR_LIGHT_INTENSITY    1
#define POINT_LIGHT_FALLOFF         1

in vec2 vOut_texcoord;

layout (binding = 0) uniform sampler2D gbufferDiffuse;
layout (binding = 1) uniform sampler2D gbufferPosition;
layout (binding = 2) uniform sampler2D gbufferNormal;

layout (location = 0) out vec4 pOut_color;

struct DirLight
{
    vec3 vs_dir;
    float strength;
    vec3 color;
};

struct PointLight
{
    vec3 vs_pos;
    float strength;
    float radius;
    vec3 color;
};

vec3 CalculateDirLight(in DirLight light, in vec3 vs_surfaceNormal, in vec3 vs_surfacePosition, 
        in vec4 surfaceColor, float specularExponent)
{
    // Light intensities
    float ambi = AMBIENT_LIGHT_INTENSITY;
    float diff = max(0, dot(vs_surfaceNormal, light.vs_dir));
    vec3 vs_lightDirR = reflect(light.vs_dir, vs_surfaceNormal);
    float spec = pow(max(0, dot(normalize(vs_surfacePosition), vs_lightDirR)), specularExponent) * SPECULAR_LIGHT_INTENSITY;

    // Accumulate with surface lightmap samples
    vec3 ambient =  ambi * surfaceColor.rgb;
    vec3 diffuse =  diff * surfaceColor.rgb;
    vec3 specular = spec * surfaceColor.aaa;

    // No attenuation for directional lights
    vec3 result = light.strength * (ambient + diffuse + specular) * light.color;
    return result;
}

float GetPointLightAttenuation(in float dist, in float radius, in float falloff)
{
    float s = dist / radius;
    float s2 = s * s;
    float result = ((1 - s2) * (1 - s2)) / (1 + falloff * s2);
    return mix(result, 0, (s >= 1.0));  // if distance is greater than radius, no light.
}

vec3 CalculatePointLight(in PointLight light, in vec3 vs_surfaceNormal, in vec3 vs_surfacePosition,
        in vec4 surfaceColor, float specularExponent)
{
    // Light intensities
    float ambi = AMBIENT_LIGHT_INTENSITY;
    vec3 vs_lightDir = normalize(vs_surfacePosition - light.vs_pos);
    float diff = max(0, dot(vs_surfaceNormal, vs_lightDir));
    vec3 vs_lightDirR = reflect(vs_lightDir, vs_surfaceNormal);
    float spec = pow(max(0, dot(normalize(vs_surfacePosition), vs_lightDirR)), specularExponent) * SPECULAR_LIGHT_INTENSITY;

    // Accumulate with surface lightmap samples
    vec3 ambient =  ambi * surfaceColor.rgb;
    vec3 diffuse =  diff * surfaceColor.rgb;
    vec3 specular = spec * surfaceColor.aaa;

    // Attenuation factor
    // Using custom function to eliminate singularities. R is radius where light is 0.
    // Reference: https://lisyarus.github.io/blog/graphics/2022/07/30/point-light-attenuation.html 
    float dist = distance(light.vs_pos, vs_surfacePosition);
    float atten = GetPointLightAttenuation(dist, light.radius, POINT_LIGHT_FALLOFF);
    float strength = light.strength * atten;

    vec3 result = strength * (ambient + diffuse + specular) * light.color;
    return result;
}

uniform DirLight u_dirLight;
uniform PointLight u_pointLights[MAX_POINT_LIGHTS];
uniform uint u_pointLightsCount;

void main()
{
    vec4 gbufferDiffuseSample = texture(gbufferDiffuse, vOut_texcoord);
    vec4 gbufferPositionSample = texture(gbufferPosition, vOut_texcoord);
    vec4 gbufferNormalSample = texture(gbufferNormal, vOut_texcoord);

    vec3 vs_surfacePosition = gbufferPositionSample.xyz;
    vec3 vs_surfaceNormal = normalize(gbufferNormalSample.xyz);
    vec4 surfaceColor = gbufferDiffuseSample;
    float specularExponent = gbufferNormalSample.a;

    vec3 outColor = vec3(0,0,0);
    outColor += CalculateDirLight(u_dirLight, vs_surfaceNormal, vs_surfacePosition, surfaceColor, specularExponent);
    for(int i = 0; i < u_pointLightsCount; i++)
    {
        outColor += CalculatePointLight(u_pointLights[i], vs_surfaceNormal, vs_surfacePosition, surfaceColor, specularExponent);
    }

    pOut_color = vec4(outColor, 1);
}
