#version 460 core

in vec3 vOut_position;
//in vec3 vOut_normal;
in vec2 vOut_texcoord;
in mat3 vOut_TBN;

layout (binding = 0) uniform sampler2D ambientMap;
layout (binding = 1) uniform sampler2D diffuseMap;
layout (binding = 2) uniform sampler2D specularMap;
layout (binding = 3) uniform sampler2D alphaMask;
layout (binding = 4) uniform sampler2D bumpMap;

uniform mat4 u_world;
uniform mat4 u_view;
uniform vec3 u_baseColor;

uniform vec3 u_ambientColor;
uniform vec3 u_diffuseColor;
uniform vec3 u_specularColor;
uniform float u_specularWeight;

layout (location = 0) out vec4 pOut_diffuse;    // Diffuse color (RGB), specular intensity (A)
layout (location = 1) out vec3 pOut_position;   // View space position (RGB)
layout (location = 2) out vec4 pOut_normal;     // View space normals (RGB), specular weight (A)

vec4 SampleNeighbor(in sampler2D inputTexture, in vec2 texCoord, in vec2 texelSize, in ivec2 neighbor)
{
    return texture(inputTexture, texCoord + vec2(neighbor.x * texelSize.x, neighbor.y * texelSize.y));
}

float RGBtoGrayscale(vec3 rgbColor)
{
    return 0.2126 * rgbColor.r + 0.7152 * rgbColor.g + 0.0722 * rgbColor.b;
}

void main()
{
    vec3 ambientColor = 0.1 * u_ambientColor;
    vec3 diffuseColor = texture(diffuseMap, vOut_texcoord).rgb * u_diffuseColor * u_baseColor;
    vec3 specularColor = texture(specularMap, vOut_texcoord).rgb * u_specularColor;
    float specularIntensity = RGBtoGrayscale(specularColor);
    pOut_diffuse = vec4(diffuseColor.rgb, specularIntensity);

    pOut_position = vOut_position;

    // Sample from bump map to get tangent space normals, then transform with TBN matrix
    // to get view space normals. RGB normal maps are converted to grayscale for supporting
    // heightmaps as well.
    vec2 bumpMapTexelSize = 1.0 / textureSize(bumpMap, 0);
    float normalKernel[9] = float[]
        (
            RGBtoGrayscale(SampleNeighbor(bumpMap, vOut_texcoord, bumpMapTexelSize, ivec2(-1,-1)).rgb),
            RGBtoGrayscale(SampleNeighbor(bumpMap, vOut_texcoord, bumpMapTexelSize, ivec2( 0,-1)).rgb),
            RGBtoGrayscale(SampleNeighbor(bumpMap, vOut_texcoord, bumpMapTexelSize, ivec2( 1,-1)).rgb),
            RGBtoGrayscale(SampleNeighbor(bumpMap, vOut_texcoord, bumpMapTexelSize, ivec2(-1, 0)).rgb),
            RGBtoGrayscale(SampleNeighbor(bumpMap, vOut_texcoord, bumpMapTexelSize, ivec2( 0, 0)).rgb),
            RGBtoGrayscale(SampleNeighbor(bumpMap, vOut_texcoord, bumpMapTexelSize, ivec2( 1, 0)).rgb),
            RGBtoGrayscale(SampleNeighbor(bumpMap, vOut_texcoord, bumpMapTexelSize, ivec2(-1, 1)).rgb),
            RGBtoGrayscale(SampleNeighbor(bumpMap, vOut_texcoord, bumpMapTexelSize, ivec2( 0, 1)).rgb),
            RGBtoGrayscale(SampleNeighbor(bumpMap, vOut_texcoord, bumpMapTexelSize, ivec2( 1, 1)).rgb)
        );

    vec3 normal;
    normal.x = -(normalKernel[2] - normalKernel[0] + 2 * (normalKernel[5] - normalKernel[3]) + normalKernel[8] - normalKernel[6]);
    normal.y = -(normalKernel[6] - normalKernel[0] + 2 * (normalKernel[7] - normalKernel[1]) + normalKernel[8] - normalKernel[2]);
    normal.z = 1.0;
    normal = normalize(normal);
    normal = normalize(vOut_TBN * normal);
    pOut_normal = vec4(normal.rgb, u_specularWeight);
}
