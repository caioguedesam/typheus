#version 460

layout (location = 0) in vec3 vIn_position;
layout (location = 1) in vec3 vIn_normal;
layout (location = 2) in vec3 vIn_texcoord;

layout (location = 0) out vec3 vOut_color;

void main()
{
    gl_Position = vec4(vIn_position, 1);
    vOut_color = vec3(vIn_texcoord.x, vIn_texcoord.y, 0);
}
