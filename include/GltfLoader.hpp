#pragma once

#include "tiny_gltf.h"

#include "Vertex.h"

struct MeshPrimitiveCPU {
    std::vector<Vertex> Vertices;
    std::vector<std::uint32_t> Indices;

    int MaterialIndex = -1;
};

struct TextureCPU {
    int Width = 0;
    int Height = 0;
    int Component = 0;
    std::vector<unsigned char> Pixels; // RGBA8 기준으로 가정
};

struct MaterialCPU {
    Vec4 BaseColorFactor = Vec4(1.f);
    float MetallicFactor = 1.f;
    float RoughnessFactor = 1.f;

    int BaseColorTexture = -1;
    int MetallicRoughnessTexture = -1;
    int NormalTexture = -1;
};

struct GltfModelCPU {
    std::vector<MeshPrimitiveCPU> Primitives;
    std::vector<TextureCPU> Textures;
    std::vector<MaterialCPU> Materials;
};

class GltfLoader {
public:
	GltfLoader();
	virtual ~GltfLoader();

public:
    static bool LoadGltfCpu(const std::string& path, GltfModelCPU& out);

private:
    // -----------------------------------------------------------------------------
    // tinygltf accessor 읽기 helpers
    // -----------------------------------------------------------------------------
    static int GetNumComponentsInType(int type);
    static int GetComponentSizeInBytes(int componentType);

    static const unsigned char* GetAccessorDataPtr(
        const tinygltf::Model& model,
        const tinygltf::Accessor& accessor,
        size_t& outStride);

    static Vec3 ReadVec3Float(
        const tinygltf::Model& model,
        const tinygltf::Accessor& accessor,
        size_t index);
    static Vec2 ReadVec2Float(
        const tinygltf::Model& model,
        const tinygltf::Accessor& accessor,
        size_t index);
    static uint32_t ReadIndex(
        const tinygltf::Model& model,
        const tinygltf::Accessor& accessor,
        size_t index);

    // -----------------------------------------------------------------------------
    // Material / Texture 변환
    // -----------------------------------------------------------------------------
    static void ConvertTextures(const tinygltf::Model& src, GltfModelCPU& dst);
    static void ConvertMaterials(const tinygltf::Model& src, GltfModelCPU& dst);

    // -----------------------------------------------------------------------------
    // Mesh primitive 변환
    // -----------------------------------------------------------------------------

    static void ConvertPrimitive(
        const tinygltf::Model& src,
        const tinygltf::Primitive& prim,
        GltfModelCPU& dst);

    // node 순회해서 mesh primitive 수집
    static void TraverseNode(
        const tinygltf::Model& src,
        int nodeIndex,
        GltfModelCPU& dst);
};