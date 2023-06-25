#version 460

layout (location = 0) in vec3 vOut_color;
layout (location = 1) in vec2 vOut_texcoord;

layout (location = 0) out vec4 pOut_color;

layout (set = 0, binding = 1) uniform sampler2D u_diffuse;

void main()
{
    // pOut_color = vec4(vOut_color, 1);
    pOut_color = texture(u_diffuse, vOut_texcoord);
}
