#include "engine/core/debug.hpp"
#include "engine/core/profile.hpp"
#include "engine/asset/asset.hpp"
#include "engine/core/file.hpp"

namespace ty
{
namespace asset
{

namespace obj
{

bool IsWhitespace(u8 p)
{
    return (p == ' ' || p == '\t' || p == '\r');
}

bool IsDigit(u8 p)
{
    return (p >= '0' && p <= '9');
}

bool IsAlpha(u8 p)
{
    return (p >= 'A' && p <= 'Z') || (p >= 'a' && p <= 'z');
}

bool IsCRLF(u8 p)
{
    return (p == 0x0a || p == 0x0d);
}

bool IsExponent(u8 p)
{
    return (p == 'e' || p == 'E');
}

// Finds the first occurrence of c, starting from p.
u8* FindChar(u8* p, u8 c)
{
    ASSERT(p);
    while(*p != c) p++;
    return p;
}

// Points to the start of the next line after p. Looks for first char after a LF char.
u8* NextLineStart(u8* p)
{
    ASSERT(p);
    while(!IsCRLF(*p)) p++;
    while(IsCRLF(*p)) p++;
    return p;
}

// Finds first char from p that is not whitespace
u8* NextNonWhitespace(u8* p)
{
    ASSERT(p);
    while(IsWhitespace(*p)) p++;
    return p;
}

// Finds first char from p that is not whitespace
u8* NextWhitespace(u8* p)
{
    ASSERT(p);
    while(!IsWhitespace(*p)) p++;
    return p;
}

// Consumes a string without any whitespaces
u8* ConsumeString(u8* p, String* out)
{
    out->data = p;
    while(!IsCRLF(*p) && !IsWhitespace(*p))
    {
        p++;
    }
    out->len = (u64)p - (u64)out->data;
    return p;
}

// Consumes a float of the form "(+/-)(number).(fraction)(e/E)(exponent)"
u8* ConsumeFloat(u8* p, f32* out)
{
    ASSERT(out);
    f32 result = 0;
    f32 sign = 1;
    f32 fraction = 0;
    i32 decimalPlace = 1;

    // Sign
    if(*p == '-')
    {
        sign = -1;
        p++;
    }
    else if(*p == '+')
    {
        p++;
    }

    // Decimal part
    while(IsDigit(*p))
    {
        result = result * 10 + (*p - '0');
        p++;
    }

    // Fractionary part
    if(*p == '.')
    {
        p++;
        while(IsDigit(*p))
        {
            fraction = fraction * 10 + (*p - '0');
            decimalPlace *= 10;
            p++;
        }
    }

    result += fraction / decimalPlace;

    // Exponent
    if(IsExponent(*p))
    {
        f32 expo = 0;
        f32 expoSign = 1;
        if(*p == '-')
        {
            expoSign = -1;
            p++;
        }
        else if(*p == '+')
        {
            p++;
        }
        while(IsDigit(*p))
        {
            expo = expo * 10 + (*p - '0');
            p++;
        }
        expo *= expoSign;
        result *= powf(10, expo);
    }

    *out = sign * result;

    return p;

}

// Consumes v3f of the form "x y z"
u8* ConsumeV3F(u8* p, f32* px, f32* py, f32* pz)
{
    ASSERT(px && py && pz);
    p = ConsumeFloat(p, px); p = NextNonWhitespace(++p);
    p = ConsumeFloat(p, py); p = NextNonWhitespace(++p);
    p = ConsumeFloat(p, pz); p = NextNonWhitespace(++p);
    return p;
}

// Consumes v2f of the form "x y"
u8* ConsumeV2F(u8* p, f32* px, f32* py)
{
    ASSERT(px && py);
    p = ConsumeFloat(p, px); p = NextNonWhitespace(++p);
    p = ConsumeFloat(p, py); p = NextNonWhitespace(++p);
    return p;
}

// Consumes a face vertex of the form "(position)(/)(texcoord)(/)(normal)"
// OBJ files have indices starting at 1, so offset by -1 when parsing.
u8* ConsumeFaceVertex(u8* p, i32* vp, i32* vn, i32* vt)
{
    // TODO(caio): Deal with negative indices

    ASSERT(p && vp && vn && vt);
    u32 currentIndex = 0;
    *vp = -1; *vn = -1; *vt = -1;
    // Position index
    while(IsDigit(*p))
    {
        currentIndex = currentIndex * 10 + (*p - '0'); p++;
    }
    *vp = currentIndex - 1; currentIndex = 0;
    // Texcoord index
    if(*p == '/')
    {
        p++;
        while(IsDigit(*p))
        {
            currentIndex = currentIndex * 10 + (*p - '0'); p++;
        }
        *vt = currentIndex - 1; currentIndex = 0;
    }
    // Normal index
    if(*p == '/')
    {
        p++;
        while(IsDigit(*p))
        {
            currentIndex = currentIndex * 10 + (*p - '0'); p++;
        }
        *vn = currentIndex - 1; currentIndex = 0;
    }

    return p;
}

u32 AddVertex(List<f32>* vertices,
        List<f32>* positions, List<f32>* normals, List<f32>* texcoords,
        i32 p, i32 n, i32 t)
{
    //TODO(caio): Vertex deduplication with hash map

    // Insert vertex position attribute
    ASSERT(p != -1);
    f32* vp = &(*positions)[p * 3];
    vertices->Push(*vp); vp++;
    vertices->Push(*vp); vp++;
    vertices->Push(*vp);

    // Insert vertex normal attribute
    if(n == -1)
    {
        vertices->Push(0);
        vertices->Push(0);
        vertices->Push(0);
    }
    else
    {
        f32* vn = &(*normals)[n * 3];
        vertices->Push(*vn); vn++;
        vertices->Push(*vn); vn++;
        vertices->Push(*vn);
    }

    // Insert vertex texcoord attribute
    if(t == -1)
    {
        vertices->Push(0);
        vertices->Push(0);
    }
    else
    {
        f32* vt = &(*texcoords)[t * 2];
        vertices->Push(*vt); vt++;
        vertices->Push(*vt);
    }

    return vertices->count / 8;
}

// Parses the .mtl for an .obj file, creates material assets,
// then returns all created materials indexed by string, to be
// used when parsing the .obj groups later
// //TODO(caio): This does not support all parameters of .mtl, e.g. emissiveness, transparency.
HashMap<String, Handle<Material>> LoadMaterials(u8* mtlData, u64 mtlDataSize)
{
    PROFILE_SCOPE;
    HashMap<String, Handle<Material>> result = MakeMap<String, Handle<Material>>(512); // Maximum of 512 materials per model

    u8* fp = mtlData;
    u8* fEnd = fp + mtlDataSize;

    Handle<Material> material = {};
    while(true)
    {
        if(fp == fEnd) break;   // EOF

        if(*fp == 'n')          // New material (newmtl)
        {
            // Skip "newmtl" and make string with material name
            fp = NextWhitespace(++fp);
            fp = NextNonWhitespace(++fp);
            String mtlName;
            fp = ConsumeString(fp, &mtlName);

            materials.Push({});
            material = { (u32)materials.count - 1 };
            result.Insert(mtlName, material);

            ////TODO(caio): Should I be indexing materials via string on loaded assets?
            //assetDatabase.loadedAssets.Insert(assetPath.str, result.value);

            fp = NextLineStart(fp);
        }
        else if(*fp == 'K')     // Colors
        {
            ASSERT(material.IsValid());
            Material& m = materials[material];

            fp++;
            f32 r = 0, g = 0, b = 0;
            if(*fp == 'a')          // Ambient
            {
                fp = NextNonWhitespace(++fp);
                fp = ConsumeV3F(fp, &r, &g, &b);
                m.ambientColor = {r, g, b};
            }
            else if(*fp == 'd')     // Diffuse
            {
                fp = NextNonWhitespace(++fp);
                fp = ConsumeV3F(fp, &r, &g, &b);
                m.diffuseColor = {r, g, b};
            }
            else if(*fp == 's')     // Specular
            {
                fp = NextNonWhitespace(++fp);
                fp = ConsumeV3F(fp, &r, &g, &b);
                m.specularColor = {r, g, b};
            }
            //else ASSERT(0);
            fp = obj::NextLineStart(fp);
        }
        else if(*fp == 'N')     // Constants
        {
            ASSERT(material.IsValid());
            Material& m = materials[material];

            fp++;
            f32 k = 0;
            if(*fp == 's')          // Specular weight
            {
                fp = NextNonWhitespace(++fp);
                fp = ConsumeFloat(fp, &k);
                m.specularExponent = k;
            }
            fp = obj::NextLineStart(fp);
        }
        else if(*fp == 'm')     // Maps
        {
            ASSERT(material.IsValid());
            Material& m = materials[material];

            fp = FindChar(fp, '_');
            fp++;
            if(*fp == 'K')
            {
                fp++;
                if(*fp == 'a')          // Ambient
                {
                    fp = NextNonWhitespace(++fp);
                    String mapName;
                    fp = ConsumeString(fp, &mapName);
                    m.ambientMap = file::MakePathAlloc(mapName);
                }
                else if(*fp == 'd')     // Diffuse
                {
                    fp = NextNonWhitespace(++fp);
                    String mapName;
                    fp = ConsumeString(fp, &mapName);
                    m.diffuseMap = file::MakePathAlloc(mapName);
                }
                else if(*fp == 's')     // Specular
                {
                    fp = NextNonWhitespace(++fp);
                    String mapName;
                    fp = ConsumeString(fp, &mapName);
                    m.specularMap = file::MakePathAlloc(mapName);
                }
                else ASSERT(0);
            }
            else if(*fp == 'b')         // Normal/bump
            {
                fp = NextWhitespace(fp);
                fp = NextNonWhitespace(fp);
                String mapName;
                fp = ConsumeString(fp, &mapName);
                m.bumpMap = file::MakePathAlloc(mapName);
            }
            else if(*fp == 'd')         // Alpha
            {
                fp = NextNonWhitespace(++fp);
                String mapName;
                fp = ConsumeString(fp, &mapName);
                m.alphaMap = file::MakePathAlloc(mapName);
            }
            fp = NextLineStart(fp);
        }
        else if(IsWhitespace(*fp))
        {
            fp = NextNonWhitespace(fp);
        }
        else
        {
            fp = NextLineStart(fp);
        }
    }

    return result;
}

}   // obj

Handle<Model> LoadModelOBJ(file::Path assetPath, bool flipVerticalTexcoord)
{
    PROFILE_SCOPE;
    if(IsLoaded(assetPath)) return { loadedAssets[assetPath.str] };
    mem::SetContext(&assetHeap);

    Model model = {};

    List<f32> positions = MakeList<f32>();
    List<f32> normals   = MakeList<f32>();
    List<f32> texcoords = MakeList<f32>();

    //TODO(caio): Vertex deduplication with hash map

    model.vertices = MakeList<f32>();
    model.groups = MakeList<ModelGroup>();

    // Read mtl data
    file::Path mtlPath = file::MakePathAlloc(assetPath.RemoveExtension().str);
    mtlPath.str.Append(".mtl");
    HashMap<String, Handle<Material>> modelMaterials;
    u64 mtlDataSize = 0;
    u8* mtlData = NULL;
    if(mtlPath.Exists())
    {
        mtlDataSize = 0;
        mtlData = file::ReadFileToBuffer(mtlPath, &mtlDataSize);
        DestroyMStr(&mtlPath.str);

        // Parse mtl data
        modelMaterials = obj::LoadMaterials(mtlData, mtlDataSize);
    }

    // Read obj data
    u64 fSize = 0;
    u8* objData = file::ReadFileToBuffer(assetPath, &fSize);

    // Parse obj data
    u8* fp = objData;
    u8* fEnd = fp + fSize;
    ModelGroup* group = NULL;
    while(true)
    {
        if(fp == fEnd) break;                       // EOF

        if(*fp == 'v')                              // Vertex attribute
        {
            fp = obj::NextNonWhitespace(++fp);
            if(*fp == 't')                          // Texture coordinate
            {
                fp = obj::NextNonWhitespace(++fp);
                f32 tx = 0, ty = 0;
                fp = obj::ConsumeV2F(fp, &tx, &ty);
                if(flipVerticalTexcoord)
                {
                    texcoords.Push(tx);
                    texcoords.Push(-ty);
                }
                else
                {
                    texcoords.Push(tx);
                    texcoords.Push(ty);
                }
            }
            else if(*fp == 'n')                     // Vertex normal
            {
                fp = obj::NextNonWhitespace(++fp);
                f32 nx = 0, ny = 0, nz = 0;
                fp = obj::ConsumeV3F(fp, &nx, &ny, &nz);
                normals.Push(nx);
                normals.Push(ny);
                normals.Push(nz);
            }
            else                                    // Vertex position
            {
                f32 px = 0, py = 0, pz = 0;
                fp = obj::ConsumeV3F(fp, &px, &py, &pz);
                positions.Push(px);
                positions.Push(py);
                positions.Push(pz);
            }
        }
        else if(*fp == 'f')                         // Face polygon
        {
            ASSERT(group);
            fp = obj::NextNonWhitespace(++fp);

            i32 face0[3] = {0,0,0};
            i32 face1[3] = {0,0,0};
            i32 face2[3] = {0,0,0};

            fp = obj::ConsumeFaceVertex(fp, &face0[0], &face0[1], &face0[2]); fp = obj::NextNonWhitespace(fp);
            fp = obj::ConsumeFaceVertex(fp, &face1[0], &face1[1], &face1[2]); fp = obj::NextNonWhitespace(fp);
            fp = obj::ConsumeFaceVertex(fp, &face2[0], &face2[1], &face2[2]); fp = obj::NextNonWhitespace(fp);
            u32 index0 = obj::AddVertex(&model.vertices, &positions, &normals, &texcoords, face0[0], face0[1], face0[2]);
            u32 index1 = obj::AddVertex(&model.vertices, &positions, &normals, &texcoords, face1[0], face1[1], face1[2]);
            u32 index2 = obj::AddVertex(&model.vertices, &positions, &normals, &texcoords, face2[0], face2[1], face2[2]);
            group->indices.Push(index0 - 1);
            group->indices.Push(index1 - 1);
            group->indices.Push(index2 - 1);
            while(!obj::IsCRLF(*fp))
            {
                face1[0] = face2[0];
                face1[1] = face2[1];
                face1[2] = face2[2];
                fp = obj::ConsumeFaceVertex(fp, &face2[0], &face2[1], &face2[2]); fp = obj::NextNonWhitespace(fp);
                index1 = index2;
                index2 = obj::AddVertex(&model.vertices, &positions, &normals, &texcoords, face2[0], face2[1], face2[2]);
                group->indices.Push(index0 - 1);
                group->indices.Push(index1 - 1);
                group->indices.Push(index2 - 1);
            }
        }
        else if(*fp == 'g' || *fp == 'o')       // Object/group
        {
            ModelGroup nextGroup = {};
            nextGroup.indices = MakeList<u32>();
            model.groups.Push(nextGroup);
            group = &model.groups[model.groups.count - 1];
            fp = obj::NextLineStart(fp);
        }
        else if(*fp == 'u')                     // Material
        {
            fp = obj::NextWhitespace(fp);
            fp = obj::NextNonWhitespace(fp);

            String mtlStr;
            fp = obj::ConsumeString(fp, &mtlStr);
            Handle<Material> mtl = modelMaterials[mtlStr];
            group->material = mtl;

            fp = obj::NextLineStart(fp);
        }
        else
        {
            fp = obj::NextLineStart(fp);
        }
    }

    // Free unneeded data (everything outside model asset)
    if(mtlData) DestroyMap(&modelMaterials);
    DestroyList(&positions);
    DestroyList(&normals);
    DestroyList(&texcoords);

    if(mtlData) mem::Free(mtlData);
    mem::Free(objData);

    models.Push(model);
    Handle<Model> result = { (u32)models.count - 1 };
    loadedAssets.Insert(assetPath.str, result.value);
    
    return result;
}

}   // asset
}   // ty
