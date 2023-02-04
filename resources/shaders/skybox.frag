#version 460 core

in vec3 vOut_texcoord;

layout (binding = 0) uniform samplerCube skyboxMap;

layout (location = 0) out vec4 pOut_color;

void main()
{
    vec3 sampleDir = vec3(-vOut_texcoord.x, vOut_texcoord.y, vOut_texcoord.z);
    pOut_color = texture(skyboxMap, sampleDir);
}
