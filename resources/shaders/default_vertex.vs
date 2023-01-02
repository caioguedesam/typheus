#version 460 core
layout (location = 0) in vec3 vIn_Position;
layout (location = 1) in vec3 vIn_Normal;
layout (location = 2) in vec2 vIn_Texcoord;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Proj;

out vec3 vOut_Position;
out vec2 vOut_Texcoord;

void main()
{
    gl_Position = u_Proj * u_View * u_Model * vec4(vIn_Position.xyz, 1.0);

    vOut_Position = vec3(gl_Position.xyz);
    vOut_Texcoord = vIn_Texcoord;
}
