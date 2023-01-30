#version 460 core

in vec2 vOut_texcoord;

layout (binding = 0) uniform sampler2D gbufferDiffuse;
layout (binding = 1) uniform sampler2D gbufferPosition;
layout (binding = 2) uniform sampler2D gbufferNormal;

layout (location = 0) out vec4 pOut_color;

void main()
{
    vec3 diffuseColor = texture(gbufferNormal, vOut_texcoord).rgb;
    pOut_color = vec4(diffuseColor.rgb, 1);
}
