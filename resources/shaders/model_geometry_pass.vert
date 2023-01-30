#version 460 core
layout (location = 0) in vec3 vIn_position;
layout (location = 1) in vec3 vIn_normal;
layout (location = 2) in vec3 vIn_tangent;
layout (location = 3) in vec3 vIn_bitangent;
layout (location = 4) in vec2 vIn_texcoord;

uniform mat4 u_world;
uniform mat4 u_view;
uniform mat4 u_proj;

out vec3 vOut_position;
//out vec3 vOut_normal;
out vec2 vOut_texcoord;
out mat3 vOut_TBN;

void main()
{
    // Vertex outputs from gbuffer are stored in view space
    mat4 VW = u_view * u_world;
    vec4 viewPosition = VW * vec4(vIn_position.xyz, 1.0);
    gl_Position = u_proj * viewPosition;

    vOut_position = vec3(viewPosition.xyz);
    //vOut_normal = transpose(inverse(mat3(VW))) * vIn_normal;
    vec3 tangentCol     = normalize(vec3(VW * vec4(vIn_tangent, 0)));
    vec3 bitangentCol   = normalize(vec3(VW * vec4(vIn_bitangent, 0)));
    vec3 normalCol      = normalize(vec3(VW * vec4(vIn_normal, 0)));
    vOut_TBN = mat3(tangentCol, bitangentCol, normalCol);
    vOut_texcoord = vIn_texcoord;
}
