#pragma once

#include "tiny_gltf.h"

#include "Vertex.h"

struct MeshPrimitiveCPU {
    std::vector<Vertex> Vertices;
    std::vector<std::uint32_t> Indices;

    // 스키닝 데이터는 Vertex와 병렬 인덱스로 접근
    std::vector<std::array<std::uint16_t, 4>> JointIndices;
    std::vector<Vec4> JointWeights;

    int MaterialIndex = -1;
    int NodeIndex = -1;
    int SkinIndex = -1;

	EVertex::Type VertexType = EVertex::E_Static;
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

struct NodeCPU {
    std::string Name;

    int ParentIndex = -1;
    std::vector<int> Children;

    int MeshIndex = -1;
    int SkinIndex = -1;

    Vec3 Translation = Vec3(0.f, 0.f, 0.f);
    Vec4 Rotation = Vec4(0.f, 0.f, 0.f, 1.f); // quaternion xyzw
    Vec3 Scale = Vec3(1.f, 1.f, 1.f);
    Mat4 LocalMatrix = Identity4x4;

    bool HasMatrix = false;
};

struct SkinCPU {
    std::string Name;
    int SkeletonRootNode = -1;
    std::vector<int> Joints;           // dst.Nodes 기준 인덱스
    std::vector<Mat4> InverseBindMatrices;
};

enum class AnimationPathCPU {
    Translation,
    Rotation,
    Scale,
    Weights,
    Unknown,
};

enum class InterpolationCPU {
    Linear,
    Step,
    CubicSpline,
};

struct AnimationSamplerCPU {
    std::vector<float> InputTimes;
    std::vector<Vec4> OutputsValues;
    InterpolationCPU Interpolation = InterpolationCPU::Linear;
    AnimationPathCPU Path = AnimationPathCPU::Unknown;
};

struct AnimationChannelCPU {
    int SamplerIndex = -1;
    int TargetNodeIndex = -1; // dst.Nodes 기준
    AnimationPathCPU Path = AnimationPathCPU::Unknown;
};

struct AnimationCPU {
    std::string Name;
    std::vector<AnimationSamplerCPU> Samplers;
    std::vector<AnimationChannelCPU> Channels;

    int SkinIndex = -1; // 가장 잘 맞는 skin
    float Duration = 0.f;
};

struct GltfMeshCPU {
    std::vector<MeshPrimitiveCPU> Primitives;
    std::vector<TextureCPU> Textures;
    std::vector<MaterialCPU> Materials;
};

struct GltfSkeletonCPU {
    std::vector<NodeCPU> Nodes;
    std::vector<int> SceneRoots;
    std::vector<SkinCPU> Skins;
};

struct GltfAnimationSetCPU {
    std::vector<AnimationCPU> Animations;
};

struct GltfLoadResultCPU {
    GltfMeshCPU Mesh;
    GltfSkeletonCPU Skeleton;
    GltfAnimationSetCPU AnimationSet;

    std::vector<int> NodeRemap;
};

class GltfLoader {
public:
	GltfLoader();
	virtual ~GltfLoader();

public:
    static bool LoadGltfCpu(const std::string& path, GltfLoadResultCPU& out);

private:
    static int GetNumComponentsInType(int type);
    static int GetComponentSizeInBytes(int componentType);

    static const unsigned char* GetAccessorDataPtr(
        const tinygltf::Model& model,
        const tinygltf::Accessor& accessor,
        size_t& outStride);

    static float ReadScalarAsFloat(
        const tinygltf::Model& model,
        const tinygltf::Accessor& accessor,
        size_t index);
    static Vec2 ReadVec2Float(
        const tinygltf::Model& model,
        const tinygltf::Accessor& accessor,
        size_t index);
    static Vec3 ReadVec3Float(
        const tinygltf::Model& model,
        const tinygltf::Accessor& accessor,
        size_t index);
    static Vec4 ReadVec4Float(
        const tinygltf::Model& model,
        const tinygltf::Accessor& accessor,
        size_t index);
    static Mat4 ReadMat4Float(
        const tinygltf::Model& model,
        const tinygltf::Accessor& accessor,
        size_t index);
    static uint32_t ReadIndex(
        const tinygltf::Model& model,
        const tinygltf::Accessor& accessor,
        size_t index);
    static std::array<std::uint16_t, 4> ReadJoints4(
        const tinygltf::Model& model,
        const tinygltf::Accessor& accessor,
        size_t index);

    static void ConvertTextures(const tinygltf::Model& src, GltfLoadResultCPU& dst);
    static void ConvertMaterials(const tinygltf::Model& src, GltfLoadResultCPU& dst);
    static void ConvertSkins(const tinygltf::Model& src, GltfLoadResultCPU& dst);
    static void ConvertAnimations(const tinygltf::Model& src, GltfLoadResultCPU& dst);

    static void ConvertPrimitive(
        const tinygltf::Model& src,
        const tinygltf::Primitive& prim,
        int nodeIndex,
        GltfLoadResultCPU& dst);

    static void ConvertPrimitive(
        const tinygltf::Model& src,
        const tinygltf::Primitive& prim,
        int nodeIndex,
        int skinIndex,
        GltfLoadResultCPU& dst);

    static int TraverseNode(
        const tinygltf::Model& src,
        int srcNodeIndex,
        int parentDstNodeIndex,
        GltfLoadResultCPU& dst);

    static int FindBestSkinForAnimation(
        const AnimationCPU& animation,
        const GltfSkeletonCPU& skeleton);

    static AnimationPathCPU ToAnimationPath(const std::string& path);
    static InterpolationCPU ToInterpolation(const std::string& interpolation);
};