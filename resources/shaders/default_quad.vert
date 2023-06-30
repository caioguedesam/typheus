#version 460

layout (location = 0) in vec3 vIn_position;
layout (location = 1) in vec3 vIn_normal;
layout (location = 2) in vec2 vIn_texcoord;

layout (location = 0) out vec2 vOut_texcoord;
layout (location = 1) out int vOut_instanceIndex;

#define INSTANCE_COUNT 512
struct PerInstanceData
{
    mat4 world;
    vec4 color;
};

layout (set = 0, binding = 0) uniform SceneDataBlock
{
    mat4 view;
    mat4 proj;
} u_scene;

layout (set = 0, binding = 2) uniform InstanceDataBlock
{
    PerInstanceData data[INSTANCE_COUNT];
} u_instances;

void main()
{
    PerInstanceData instanceData = u_instances.data[gl_InstanceIndex];
    gl_Position = u_scene.proj * u_scene.view * instanceData.world * vec4(vIn_position, 1);
    vOut_texcoord = vIn_texcoord;
    vOut_instanceIndex = gl_InstanceIndex;
}
