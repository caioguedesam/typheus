#version 460 core

#define MAX_POINT_LIGHTS 16
#define AMBIENT_LIGHT_INTENSITY     0.1
#define SPECULAR_LIGHT_INTENSITY    1
#define POINT_LIGHT_FALLOFF         1
#define PCF_SAMPLE_DIMENSION        5

in vec2 vOut_texcoord;

layout (binding = 0) uniform sampler2D gbufferDiffuse;
layout (binding = 1) uniform sampler2D gbufferPosition;
layout (binding = 2) uniform sampler2D gbufferNormal;
layout (binding = 3) uniform sampler2D shadowMap;

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

float CalculateShadow(in vec4 ls_surfacePosition)
{
    // Perspective divide to clip space
    vec3 cs_position = ls_surfacePosition.xyz / ls_surfacePosition.w;
    // Then from NDC [-1, 1] to shadow map sample space [0, 1]
    cs_position = cs_position * 0.5 + 0.5;

    if(cs_position.z > 1.0) return 0;   // Points farther than the light's far plane are not shadowed.
    
    float bias = 0.005;
    float result = 0;
    // Percentage-closer filtering
    vec2 shadowMapTexelSize = 1.0 / textureSize(shadowMap, 0);
    int start = int(ceil(-PCF_SAMPLE_DIMENSION / 2.0));
    for(int x = 0; x < PCF_SAMPLE_DIMENSION; x++)
    {
        for(int y = 0; y < PCF_SAMPLE_DIMENSION; y++)
        {
            vec2 pcfOffset = vec2(x + start, y + start);
            float pcfDepth = texture(shadowMap, cs_position.xy + pcfOffset * shadowMapTexelSize).r;
            result += cs_position.z - bias > pcfDepth ? 1 : 0;
        }
    }
    result /= (PCF_SAMPLE_DIMENSION * PCF_SAMPLE_DIMENSION);
    return result;
}

vec3 CalculateDirLight(in DirLight light, in vec3 vs_surfaceNormal, in vec3 vs_surfacePosition, 
        in vec4 surfaceColor, float specularExponent, float shadow)
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
    vec3 result = light.strength * (ambient + (1.0 - shadow) * (diffuse + specular)) * light.color;
    return result;
}

float GetPointLightAttenuation(in float dist, in float radius, in float falloff)
{
    float s = dist / radius;
    float s2 = s * s;
    float result = ((1 - s2) * (1 - s2)) / (1 + falloff * s2);
    return s >= 1.0 ? 0 : result;
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
uniform mat4 u_viewToLight;

void main()
{
    vec4 gbufferDiffuseSample = texture(gbufferDiffuse, vOut_texcoord);
    vec4 gbufferPositionSample = texture(gbufferPosition, vOut_texcoord);
    vec4 gbufferNormalSample = texture(gbufferNormal, vOut_texcoord);

    vec3 vs_surfacePosition = gbufferPositionSample.xyz;
    vec3 vs_surfaceNormal = normalize(gbufferNormalSample.xyz);
    vec4 surfaceColor = gbufferDiffuseSample;
    float specularExponent = gbufferNormalSample.a;

    // Calculate shadow
    vec4 ls_surfacePosition = u_viewToLight * vec4(vs_surfacePosition, 1);
    float shadow = CalculateShadow(ls_surfacePosition);

    vec3 outColor = vec3(0,0,0);
    outColor += CalculateDirLight(u_dirLight, vs_surfaceNormal, vs_surfacePosition, surfaceColor, specularExponent, shadow);
    for(int i = 0; i < u_pointLightsCount; i++)
    {
        outColor += CalculatePointLight(u_pointLights[i], vs_surfaceNormal, vs_surfacePosition, surfaceColor, specularExponent);
    }

    pOut_color = vec4(outColor, 1);
}
