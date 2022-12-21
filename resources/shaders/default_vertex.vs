#version 460 core
layout (location = 0) in vec3 vIn_Position;
layout (location = 1) in vec3 vIn_Normal;
layout (location = 2) in vec3 vIn_Texcoord;

out vec3 vOut_Normal;

void main()
{
    gl_Position = vec4(vIn_Position.xyz, 1.0);  // Needs view and projection transform
    vOut_Normal = vIn_Normal;
}
