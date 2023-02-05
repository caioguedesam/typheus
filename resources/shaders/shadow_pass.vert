#version 460 core

layout (location = 0) in vec3 vIn_position;

uniform mat4 u_worldToLight;
uniform mat4 u_world;

void main()
{
    gl_Position = u_worldToLight * u_world * vec4(vIn_position, 1);
}
