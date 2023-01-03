#version 460 core
layout (location = 0) in vec3 vIn_Position;
layout (location = 1) in vec3 vIn_Normal;
layout (location = 2) in vec2 vIn_Texcoord;

out vec2 vOut_Texcoord;

void main()
{
    gl_Position = vec4(vIn_Position.xyz, 1.0);
    vOut_Texcoord = vIn_Texcoord;
}
