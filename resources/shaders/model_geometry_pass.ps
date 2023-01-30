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

layout (location = 0) out vec4 pOut_diffuse;
layout (location = 1) out vec3 pOut_position;
layout (location = 2) out vec3 pOut_normal;

void main()
{
    vec3 diffuseColor = texture(diffuseMap, vOut_texcoord).rgb * u_baseColor;
    pOut_diffuse = vec4(diffuseColor.rgb, 1);

    pOut_position = vOut_position;
    pOut_normal = normalize(vOut_normal);
}
