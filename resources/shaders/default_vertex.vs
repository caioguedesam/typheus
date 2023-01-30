#version 460 core
layout (location = 0) in vec3 vIn_position;
layout (location = 1) in vec3 vIn_normal;
layout (location = 2) in vec2 vIn_texcoord;

uniform mat4 u_world;
uniform mat4 u_view;
uniform mat4 u_proj;

out vec3 vOut_position;
out vec2 vOut_texcoord;

void main()
{
    gl_Position = u_proj * u_view * u_world * vec4(vIn_position.xyz, 1.0);

    vOut_position = vec3(gl_Position.xyz);
    vOut_texcoord = vIn_texcoord;
}
