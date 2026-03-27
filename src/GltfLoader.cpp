#include "pch.h"
#include "GltfLoader.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

GltfLoader::GltfLoader() {}

GltfLoader::~GltfLoader() {}

bool GltfLoader::LoadGltfCpu(const std::string& path, GltfModelCPU& out) {
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string warn, err;

    bool ok = false;
    if (path.ends_with(".glb"))
        ok = loader.LoadBinaryFromFile(&model, &err, &warn, path);
    else if (path.ends_with(".gltf"))
        ok = loader.LoadASCIIFromFile(&model, &err, &warn, path);
	else ReturnFalse("Unsupported file extension for glTF: " + path);

    if (!warn.empty())
        std::cout << "[tinygltf warn] " << warn << "\n";

    if (!err.empty())
        std::cerr << "[tinygltf err] " << err << "\n";

    if (!ok)
        ReturnFalse("Failed to load glTF");

    ConvertTextures(model, out);
    ConvertMaterials(model, out);

    int sceneIndex = model.defaultScene >= 0 ? model.defaultScene : 0;
    if (sceneIndex >= 0 && sceneIndex < (int)model.scenes.size()) {
        const tinygltf::Scene& scene = model.scenes[sceneIndex];
        for (int nodeIndex : scene.nodes)
            TraverseNode(model, nodeIndex, out);
    }

    return true;
}

// -----------------------------------------------------------------------------
// tinygltf accessor 읽기 helpers
// -----------------------------------------------------------------------------

int GltfLoader::GetNumComponentsInType(int type) {
    switch (type) {
    case TINYGLTF_TYPE_SCALAR: return 1;
    case TINYGLTF_TYPE_VEC2:   return 2;
    case TINYGLTF_TYPE_VEC3:   return 3;
    case TINYGLTF_TYPE_VEC4:   return 4;
    case TINYGLTF_TYPE_MAT2:   return 4;
    case TINYGLTF_TYPE_MAT3:   return 9;
    case TINYGLTF_TYPE_MAT4:   return 16;
    default: return 0;
    }
}

int GltfLoader::GetComponentSizeInBytes(int componentType) {
    switch (componentType) {
    case TINYGLTF_COMPONENT_TYPE_BYTE:           return 1;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:  return 1;
    case TINYGLTF_COMPONENT_TYPE_SHORT:          return 2;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return 2;
    case TINYGLTF_COMPONENT_TYPE_INT:            return 4;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:   return 4;
    case TINYGLTF_COMPONENT_TYPE_FLOAT:          return 4;
    case TINYGLTF_COMPONENT_TYPE_DOUBLE:         return 8;
    default: return 0;
    }
}

const unsigned char* GltfLoader::GetAccessorDataPtr(
    const tinygltf::Model& model
    , const tinygltf::Accessor& accessor
    , size_t& outStride) {
    const tinygltf::BufferView& bv = model.bufferViews.at(accessor.bufferView);
    const tinygltf::Buffer& buf = model.buffers.at(bv.buffer);

    const size_t componentSize = GetComponentSizeInBytes(accessor.componentType);
    const size_t numComponents = GetNumComponentsInType(accessor.type);
    const size_t elementSize = componentSize * numComponents;

    outStride = accessor.ByteStride(bv);
    if (outStride == 0)
        outStride = elementSize;

    return buf.data.data() + bv.byteOffset + accessor.byteOffset;
}

Vec3 GltfLoader::ReadVec3Float(
    const tinygltf::Model& model
    , const tinygltf::Accessor& accessor
    , size_t index) {
    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
    assert(accessor.type == TINYGLTF_TYPE_VEC3);

    size_t stride = 0;
    const unsigned char* src = GetAccessorDataPtr(model, accessor, stride);
    const float* f = reinterpret_cast<const float*>(src + stride * index);
    return Vec3(f[0], f[1], f[2]);
}

Vec2 GltfLoader::ReadVec2Float(
    const tinygltf::Model& model
    , const tinygltf::Accessor& accessor
    , size_t index) {
    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
    assert(accessor.type == TINYGLTF_TYPE_VEC2);

    size_t stride = 0;
    const unsigned char* src = GetAccessorDataPtr(model, accessor, stride);
    const float* f = reinterpret_cast<const float*>(src + stride * index);
    return Vec2(f[0], f[1]);
}

uint32_t GltfLoader::ReadIndex(
    const tinygltf::Model& model
    , const tinygltf::Accessor& accessor
    , size_t index) {
    assert(accessor.type == TINYGLTF_TYPE_SCALAR);

    size_t stride = 0;
    const unsigned char* src = GetAccessorDataPtr(model, accessor, stride);
    const unsigned char* p = src + stride * index;

    switch (accessor.componentType) {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        return *reinterpret_cast<const uint8_t*>(p);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        return *reinterpret_cast<const uint16_t*>(p);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        return *reinterpret_cast<const uint32_t*>(p);
    default:
        throw std::runtime_error("Unsupported index component type");
    }
}

// -----------------------------------------------------------------------------
// Material / Texture 변환
// -----------------------------------------------------------------------------

void GltfLoader::ConvertTextures(const tinygltf::Model& src, GltfModelCPU& dst) {
    dst.Textures.resize(src.images.size());

    for (size_t i = 0; i < src.images.size(); ++i) {
        const tinygltf::Image& img = src.images[i];
        TextureCPU tex{};

        tex.Width = img.width;
        tex.Height = img.height;
        tex.Component = img.component;
        tex.Pixels = img.image; // tinygltf가 decode한 raw image bytes

        dst.Textures[i] = std::move(tex);
    }
}

void GltfLoader::ConvertMaterials(const tinygltf::Model& src, GltfModelCPU& dst) {
    dst.Materials.resize(src.materials.size());

    for (size_t i = 0; i < src.materials.size(); ++i) {
        const tinygltf::Material& m = src.materials[i];
        MaterialCPU out{};

        const auto& pbr = m.pbrMetallicRoughness;

        if (pbr.baseColorFactor.size() == 4) {
            out.BaseColorFactor = Vec4(
                (float)pbr.baseColorFactor[0],
                (float)pbr.baseColorFactor[1],
                (float)pbr.baseColorFactor[2],
                (float)pbr.baseColorFactor[3]);
        }

        out.MetallicFactor = (float)pbr.metallicFactor;
        out.RoughnessFactor = (float)pbr.roughnessFactor;

        if (pbr.baseColorTexture.index >= 0) {
            int texIndex = src.textures[pbr.baseColorTexture.index].source;
            out.BaseColorTexture = texIndex;
        }

        if (pbr.metallicRoughnessTexture.index >= 0) {
            int texIndex = src.textures[pbr.metallicRoughnessTexture.index].source;
            out.MetallicRoughnessTexture = texIndex;
        }

        if (m.normalTexture.index >= 0) {
            int texIndex = src.textures[m.normalTexture.index].source;
            out.NormalTexture = texIndex;
        }

        dst.Materials[i] = out;
    }
}

void GltfLoader::ConvertPrimitive(
    const tinygltf::Model& src
    , const tinygltf::Primitive& prim
    , GltfModelCPU& dst) {
    if (prim.mode != TINYGLTF_MODE_TRIANGLES)
        return; // 일단 삼각형만 처리

    auto itPos = prim.attributes.find("POSITION");
    if (itPos == prim.attributes.end())
        throw std::runtime_error("Primitive has no POSITION");

    const tinygltf::Accessor& posAcc = src.accessors.at(itPos->second);
    const size_t vertexCount = posAcc.count;

    const tinygltf::Accessor* normalAcc = nullptr;
    const tinygltf::Accessor* uvAcc = nullptr;

    auto itNormal = prim.attributes.find("NORMAL");
    if (itNormal != prim.attributes.end())
        normalAcc = &src.accessors.at(itNormal->second);

    auto itUV = prim.attributes.find("TEXCOORD_0");
    if (itUV != prim.attributes.end())
        uvAcc = &src.accessors.at(itUV->second);

    MeshPrimitiveCPU out{};
    out.Vertices.resize(vertexCount);
    out.MaterialIndex = prim.material;

    for (size_t v = 0; v < vertexCount; ++v) {
        Vertex vert{};
        vert.Position = ReadVec3Float(src, posAcc, v);

        if (normalAcc)
            vert.Normal = ReadVec3Float(src, *normalAcc, v);
        else
            vert.Normal = Vec3(0.f, 1.f, 0.f);

        if (uvAcc)
            vert.TexCoord = ReadVec2Float(src, *uvAcc, v);
        else
            vert.TexCoord = Vec2(0.f);

        // glTF UV 원점/샘플링 규칙과 네 셰이더 규칙 차이에 따라 뒤집을 수 있음
        vert.TexCoord.y = 1.f - vert.TexCoord.y;
        out.Vertices[v] = vert;
    }

    if (prim.indices >= 0) {
        const tinygltf::Accessor& idxAcc = src.accessors.at(prim.indices);
        out.Indices.resize(idxAcc.count);

        for (size_t i = 0; i < idxAcc.count; ++i)
            out.Indices[i] = ReadIndex(src, idxAcc, i);
    }
    else {
        out.Indices.resize(vertexCount);
        for (uint32_t i = 0; i < (uint32_t)vertexCount; ++i)
            out.Indices[i] = i;
    }

    dst.Primitives.push_back(std::move(out));
}

// node 순회해서 mesh primitive 수집
void GltfLoader::TraverseNode(
    const tinygltf::Model& src
    , int nodeIndex
    , GltfModelCPU& dst) {
    const tinygltf::Node& node = src.nodes.at(nodeIndex);

    if (node.mesh >= 0) {
        const tinygltf::Mesh& mesh = src.meshes.at(node.mesh);
        for (const tinygltf::Primitive& prim : mesh.primitives)
            ConvertPrimitive(src, prim, dst);
    }

    for (int child : node.children)
        TraverseNode(src, child, dst);
}