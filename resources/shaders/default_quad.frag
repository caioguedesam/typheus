#version 460

layout (location = 0) in vec2 vOut_texcoord;
layout (location = 1) flat in int vOut_instanceIndex;

layout (location = 0) out vec4 pOut_color;

struct PerInstanceData
{
    mat4 world;
    vec4 color;
};

layout (set = 0, binding = 1) uniform sampler2D u_diffuse;
//layout (set = 0, binding = 2) uniform InstanceDataBlock
layout (std140, set = 0, binding = 2) readonly buffer InstanceDataBlock
{
    PerInstanceData data[];
} u_instances;

void main()
{
    PerInstanceData instanceData = u_instances.data[vOut_instanceIndex];
    pOut_color = instanceData.color * texture(u_diffuse, vOut_texcoord);
}
