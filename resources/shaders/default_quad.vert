#version 460

layout (location = 0) in vec3 vIn_position;
layout (location = 1) in vec3 vIn_normal;
layout (location = 2) in vec2 vIn_texcoord;

layout (location = 0) out vec3 vOut_color;
layout (location = 1) out vec2 vOut_texcoord;

layout (set = 0, binding = 0) uniform SceneData
{
    mat4 view;
    mat4 proj;
} u_scene;

layout (set = 1, binding = 0) uniform ObjectData
{
    mat4 world;
} u_object;

// layout (set = 0, binding = 0) uniform UniformBuffer
// {
//     mat4 world;
//     mat4 view;
//     mat4 projection;
// } u_data;

void main()
{
    // gl_Position = u_data.projection * u_data.view * u_data.world * vec4(vIn_position, 1);
    gl_Position = u_scene.proj * u_scene.view * u_object.world * vec4(vIn_position, 1);
    vOut_color = vec3(vIn_texcoord.x, vIn_texcoord.y, 0);
    vOut_texcoord = vIn_texcoord;
}
