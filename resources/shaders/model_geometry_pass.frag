#version 460 core

in vec3 vOut_position;
in vec3 vOut_normal;
in vec2 vOut_texcoord;

layout (binding = 0) uniform sampler2D ambientMap;
layout (binding = 1) uniform sampler2D diffuseMap;
layout (binding = 2) uniform sampler2D specularMap;
layout (binding = 3) uniform sampler2D alphaMask;
layout (binding = 4) uniform sampler2D bumpMap;

uniform mat4 u_world;
uniform mat4 u_view;
uniform vec3 u_baseColor;

uniform vec3 u_ambientColor;
uniform vec3 u_diffuseColor;
uniform vec3 u_specularColor;
uniform float u_specularWeight;

layout (location = 0) out vec4 pOut_diffuse;    // Diffuse color (RGB), specular intensity (A)
layout (location = 1) out vec3 pOut_position;   // View space position (RGB)
layout (location = 2) out vec4 pOut_normal;     // View space normals (RGB), specular weight (A)

void main()
{
    vec3 ambientColor = 0.1 * u_ambientColor;
    vec3 diffuseColor = texture(diffuseMap, vOut_texcoord).rgb * u_diffuseColor * u_baseColor;
    vec3 specularColor = texture(specularMap, vOut_texcoord).rgb * u_specularColor;
    float specularIntensity = 0.2126 * specularColor.r + 0.7152 * specularColor.g + 0.0722 * specularColor.b;
    pOut_diffuse = vec4(diffuseColor.rgb, specularIntensity);

    pOut_position = vOut_position;
    pOut_normal = vec4(normalize(vOut_normal).rgb, u_specularWeight);
}
