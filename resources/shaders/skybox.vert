#version 460 core
layout (location = 0) in vec3 vIn_position;

uniform mat4 u_view;
uniform mat4 u_proj;

out vec3 vOut_texcoord;

void main()
{
    vec4 cs_position = u_proj * mat4(mat3(u_view)) * vec4(vIn_position, 1.0);
    gl_Position = cs_position.xyww; // Draws skybox cube at max depth

    vOut_texcoord = vIn_position;
}
