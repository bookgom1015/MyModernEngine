#include "pch.h"
#include "GltfLoader.hpp"

#include "EditorManager.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

namespace {
    template <typename T>
    T ClampCastUintTo(const uint32_t value) {
        return static_cast<T>(std::min<uint32_t>(value, static_cast<uint32_t>(std::numeric_limits<T>::max())));
    }

    bool IsSkinnedNode(const tinygltf::Node& node) {
        return node.skin >= 0;
    }

    bool IsSkinnedPrimitive(const tinygltf::Primitive& primitive) {
        const auto& attrs = primitive.attributes;

        return (attrs.find("JOINTS_0") != attrs.end()) 
            && (attrs.find("WEIGHTS_0") != attrs.end());
    }

    bool IsSkinnedMesh(
        const tinygltf::Model& model
        , const tinygltf::Node& node
        , const tinygltf::Mesh& mesh) {
        // 1. primitive에 joint/weight 있는지
        for (const auto& primitive : mesh.primitives) 
            if (IsSkinnedPrimitive(primitive))
                return true;

        // 2. node에 skin이 연결되어 있는지
        if (IsSkinnedNode(node))
            return true;

        return false;
    }
}

GltfLoader::GltfLoader() {}

GltfLoader::~GltfLoader() {}

bool GltfLoader::LoadGltfCpu(const std::string& path, GltfLoadResultCPU& out) {
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string warn, err;

    bool ok = false;
    if (path.ends_with(".glb"))
        ok = loader.LoadBinaryFromFile(&model, &err, &warn, path);
    else if (path.ends_with(".gltf"))
        ok = loader.LoadASCIIFromFile(&model, &err, &warn, path);
    else
        ReturnFalse("Unsupported file extension for glTF: " + path);

    if (!warn.empty()) {
        auto msg = std::format("tinygltf warning: {}", warn);
        LOG_WARNING(msg);
    }

    if (!err.empty()) {
        auto msg = std::format("tinygltf error: {}", err);
        LOG_ERROR(msg);
        Logln(msg);
    }

    if (!ok)
        ReturnFalse("Failed to load glTF");

    out = {};
    out.NodeRemap.assign(model.nodes.size(), -1);

    ConvertTextures(model, out);
    ConvertMaterials(model, out);

    const int sceneIndex = model.defaultScene >= 0 ? model.defaultScene : 0;
    if (sceneIndex >= 0 && sceneIndex < static_cast<int>(model.scenes.size())) {
        const tinygltf::Scene& scene = model.scenes[sceneIndex];

        for (const int srcRootNodeIndex : scene.nodes) {
            const int dstRoot = TraverseNode(model, srcRootNodeIndex, -1, out);
            out.Skeleton.SceneRoots.push_back(dstRoot);
        }
    }

    ConvertSkins(model, out);
    ConvertAnimations(model, out);

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

float GltfLoader::ReadScalarAsFloat(
    const tinygltf::Model& model
    , const tinygltf::Accessor& accessor
    , size_t index) {
    assert(accessor.type == TINYGLTF_TYPE_SCALAR);

    size_t stride = 0;
    const unsigned char* src = GetAccessorDataPtr(model, accessor, stride);
    const unsigned char* p = src + stride * index;

    switch (accessor.componentType) {
    case TINYGLTF_COMPONENT_TYPE_FLOAT:
        return *reinterpret_cast<const float*>(p);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        return static_cast<float>(*reinterpret_cast<const uint8_t*>(p));
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        return static_cast<float>(*reinterpret_cast<const uint16_t*>(p));
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        return static_cast<float>(*reinterpret_cast<const uint32_t*>(p));
    default:
        throw std::runtime_error("Unsupported scalar component type");
    }
}

Vec2 GltfLoader::ReadVec2Float(
    const tinygltf::Model& model
    , const tinygltf::Accessor& accessor
    , size_t index) {
    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
    assert(accessor.type == TINYGLTF_TYPE_VEC2);

    size_t stride = 0;
    const unsigned char* src = GetAccessorDataPtr(model, accessor, stride);
    const unsigned char* p = src + stride * index;

    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
        const float* f = reinterpret_cast<const float*>(p);
        return Vec2(f[0], f[1]);
    }

    throw std::runtime_error("Unsupported vec2 component type");
}

Vec3 GltfLoader::ReadVec3Float(
    const tinygltf::Model& model
    , const tinygltf::Accessor& accessor
    , size_t index) {
    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
    assert(accessor.type == TINYGLTF_TYPE_VEC3);

    size_t stride = 0;
    const unsigned char* src = GetAccessorDataPtr(model, accessor, stride);
    const unsigned char* p = src + stride * index;

    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
        const float* f = reinterpret_cast<const float*>(p);
        return Vec3(f[0], f[1], f[2]);
    }

    throw std::runtime_error("Unsupported vec3 component type");
}

Vec4 GltfLoader::ReadVec4Float(
    const tinygltf::Model& model
    , const tinygltf::Accessor& accessor
    , size_t index) {
    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
    assert(accessor.type == TINYGLTF_TYPE_VEC4);

    size_t stride = 0;
    const unsigned char* src = GetAccessorDataPtr(model, accessor, stride);
    const unsigned char* p = src + stride * index;

    switch (accessor.componentType) {
    case TINYGLTF_COMPONENT_TYPE_FLOAT: {
        const float* f = reinterpret_cast<const float*>(p);
        return Vec4(f[0], f[1], f[2], f[3]);
    }
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
        const uint8_t* u = reinterpret_cast<const uint8_t*>(p);
        if (accessor.normalized)
            return Vec4(u[0] / 255.f, u[1] / 255.f, u[2] / 255.f, u[3] / 255.f);
        return Vec4(static_cast<float>(u[0]), static_cast<float>(u[1]), static_cast<float>(u[2]), static_cast<float>(u[3]));
    }
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
        const uint16_t* u = reinterpret_cast<const uint16_t*>(p);
        if (accessor.normalized)
            return Vec4(u[0] / 65535.f, u[1] / 65535.f, u[2] / 65535.f, u[3] / 65535.f);
        return Vec4(static_cast<float>(u[0]), static_cast<float>(u[1]), static_cast<float>(u[2]), static_cast<float>(u[3]));
    }
    default:
        throw std::runtime_error("Unsupported vec4 component type");
    }
}

Mat4 GltfLoader::ReadMat4Float(
    const tinygltf::Model& model
    , const tinygltf::Accessor& accessor
    , size_t index) {
    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
    assert(accessor.type == TINYGLTF_TYPE_MAT4);

    size_t stride = 0;
    const unsigned char* src = GetAccessorDataPtr(model, accessor, stride);
    const float* f = reinterpret_cast<const float*>(src + stride * index);

    return Mat4(
        f[0], f[1], f[2], f[3],
        f[4], f[5], f[6], f[7],
        f[8], f[9], f[10], f[11],
        f[12], f[13], f[14], f[15]);
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

std::array<std::uint16_t, 4> GltfLoader::ReadJoints4(
    const tinygltf::Model& model
    , const tinygltf::Accessor& accessor
    , size_t index) {
    assert(accessor.type == TINYGLTF_TYPE_VEC4);
    assert(
        accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE ||
        accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT
    );

    size_t stride = 0;
    const unsigned char* src = GetAccessorDataPtr(model, accessor, stride);
    const unsigned char* p = src + stride * index;

    std::array<std::uint16_t, 4> out{};
    switch (accessor.componentType) {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
        const uint8_t* u = reinterpret_cast<const uint8_t*>(p);
        out[0] = u[0]; out[1] = u[1]; out[2] = u[2]; out[3] = u[3];
        return out;
    }
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
        const uint16_t* u = reinterpret_cast<const uint16_t*>(p);
        out[0] = u[0]; out[1] = u[1]; out[2] = u[2]; out[3] = u[3];
        return out;
    }
    default:
        throw std::runtime_error("Unsupported JOINTS_0 component type");
    }
}

// -----------------------------------------------------------------------------
// Material / Texture 변환
// -----------------------------------------------------------------------------

void GltfLoader::ConvertTextures(const tinygltf::Model& src, GltfLoadResultCPU& dst) {
    dst.Mesh.Textures.resize(src.images.size());

    for (size_t i = 0; i < src.images.size(); ++i) {
        const tinygltf::Image& img = src.images[i];
        TextureCPU tex{};

        tex.Width = img.width;
        tex.Height = img.height;
        tex.Component = img.component;
        tex.Pixels = img.image;

        dst.Mesh.Textures[i] = std::move(tex);
    }
}

void GltfLoader::ConvertMaterials(const tinygltf::Model& src, GltfLoadResultCPU& dst) {
    dst.Mesh.Materials.resize(src.materials.size());

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

        if (pbr.baseColorTexture.index >= 0)
            out.BaseColorTexture = src.textures[pbr.baseColorTexture.index].source;

        if (pbr.metallicRoughnessTexture.index >= 0)
            out.MetallicRoughnessTexture = src.textures[pbr.metallicRoughnessTexture.index].source;

        if (m.normalTexture.index >= 0)
            out.NormalTexture = src.textures[m.normalTexture.index].source;

        dst.Mesh.Materials[i] = out;
    }
}

// -----------------------------------------------------------------------------
// Animation / Skin 변환
// -----------------------------------------------------------------------------
void GltfLoader::ConvertSkins(const tinygltf::Model& src, GltfLoadResultCPU& dst) {
    dst.Skeleton.Skins.resize(src.skins.size());

    for (size_t i = 0; i < src.skins.size(); ++i) {
        const tinygltf::Skin& skin = src.skins[i];
        SkinCPU out{};
        out.Name = skin.name;

        if (skin.skeleton >= 0 && skin.skeleton < static_cast<int>(dst.NodeRemap.size()))
            out.SkeletonRootNode = dst.NodeRemap[skin.skeleton];

        out.Joints.reserve(skin.joints.size());
        for (const int srcJoint : skin.joints) {
            if (srcJoint >= 0 && srcJoint < static_cast<int>(dst.NodeRemap.size()))
                out.Joints.push_back(dst.NodeRemap[srcJoint]);
            else
                out.Joints.push_back(-1);
        }

        if (skin.inverseBindMatrices >= 0) {
            const tinygltf::Accessor& acc = src.accessors.at(skin.inverseBindMatrices);
            out.InverseBindMatrices.resize(acc.count);
            for (size_t m = 0; m < acc.count; ++m)
                out.InverseBindMatrices[m] = ReadMat4Float(src, acc, m);
        }
        else {
            out.InverseBindMatrices.resize(out.Joints.size(), Identity4x4);
        }

        dst.Skeleton.Skins[i] = std::move(out);
    }
}

void GltfLoader::ConvertAnimations(const tinygltf::Model& src, GltfLoadResultCPU& dst) {
    dst.AnimationSet.Animations.resize(src.animations.size());

    for (size_t animIndex = 0; animIndex < src.animations.size(); ++animIndex) {
        const tinygltf::Animation& anim = src.animations[animIndex];
        AnimationCPU out{};
        out.Name = anim.name;
        out.Samplers.resize(anim.samplers.size());
        out.Channels.resize(anim.channels.size());

        for (size_t channelIndex = 0; channelIndex < anim.channels.size(); ++channelIndex) {
            const tinygltf::AnimationChannel& channel = anim.channels[channelIndex];
            AnimationChannelCPU outChannel{};
            outChannel.SamplerIndex = channel.sampler;
            outChannel.Path = ToAnimationPath(channel.target_path);
            outChannel.TargetNodeIndex =
                (channel.target_node >= 0 && channel.target_node < static_cast<int>(dst.NodeRemap.size()))
                ? dst.NodeRemap[channel.target_node]
                : -1;

            out.Channels[channelIndex] = outChannel;
        }

        for (size_t samplerIndex = 0; samplerIndex < anim.samplers.size(); ++samplerIndex) {
            const tinygltf::AnimationSampler& sampler = anim.samplers[samplerIndex];
            AnimationSamplerCPU outSampler{};
            outSampler.Interpolation = ToInterpolation(sampler.interpolation);

            for (const AnimationChannelCPU& ch : out.Channels) {
                if (ch.SamplerIndex == static_cast<int>(samplerIndex)) {
                    outSampler.Path = ch.Path;
                    break;
                }
            }

            const tinygltf::Accessor& inputAcc = src.accessors.at(sampler.input);
            outSampler.InputTimes.resize(inputAcc.count);
            for (size_t k = 0; k < inputAcc.count; ++k)
                outSampler.InputTimes[k] = ReadScalarAsFloat(src, inputAcc, k);

            if (!outSampler.InputTimes.empty())
                out.Duration = std::max(out.Duration, outSampler.InputTimes.back());

            const tinygltf::Accessor& outputAcc = src.accessors.at(sampler.output);
            outSampler.OutputsValues.resize(outputAcc.count);

            switch (outSampler.Path) {
            case AnimationPathCPU::Translation:
            case AnimationPathCPU::Scale:
                for (size_t k = 0; k < outputAcc.count; ++k) {
                    const Vec3 v = ReadVec3Float(src, outputAcc, k);
                    outSampler.OutputsValues[k] = Vec4(v.x, v.y, v.z, 0.f);
                }
                break;

            case AnimationPathCPU::Rotation:
                for (size_t k = 0; k < outputAcc.count; ++k)
                    outSampler.OutputsValues[k] = ReadVec4Float(src, outputAcc, k);
                break;

            case AnimationPathCPU::Weights: {
                const int componentCount = GetNumComponentsInType(outputAcc.type);

                if (componentCount == 1) {
                    for (size_t k = 0; k < outputAcc.count; ++k) {
                        const float w = ReadScalarAsFloat(src, outputAcc, k);
                        outSampler.OutputsValues[k] = Vec4(w, 0.f, 0.f, 0.f);
                    }
                }
                else if (componentCount == 4) {
                    for (size_t k = 0; k < outputAcc.count; ++k)
                        outSampler.OutputsValues[k] = ReadVec4Float(src, outputAcc, k);
                }
                else {
                    throw std::runtime_error("Unsupported animation weights accessor type");
                }
                break;
            }

            default:
                break;
            }

            out.Samplers[samplerIndex] = std::move(outSampler);
        }

        out.SkinIndex = FindBestSkinForAnimation(out, dst.Skeleton);
        dst.AnimationSet.Animations[animIndex] = std::move(out);
    }
}

// -----------------------------------------------------------------------------
// Static Mesh primitive 변환
// -----------------------------------------------------------------------------
void GltfLoader::ConvertPrimitive(
    const tinygltf::Model& src
    , const tinygltf::Primitive& prim
    , int nodeIndex
    , GltfLoadResultCPU& dst) {
    if (prim.mode != TINYGLTF_MODE_TRIANGLES)
        return;

    auto itPos = prim.attributes.find("POSITION");
    if (itPos == prim.attributes.end())
        throw std::runtime_error("Primitive has no POSITION");

    const tinygltf::Accessor& posAcc = src.accessors.at(itPos->second);
    const size_t vertexCount = posAcc.count;

    const tinygltf::Accessor* normalAcc = nullptr;
    const tinygltf::Accessor* uvAcc = nullptr;

    if (auto it = prim.attributes.find("NORMAL"); it != prim.attributes.end())
        normalAcc = &src.accessors.at(it->second);

    if (auto it = prim.attributes.find("TEXCOORD_0"); it != prim.attributes.end())
        uvAcc = &src.accessors.at(it->second);

    MeshPrimitiveCPU out{};
    out.Vertices.resize(vertexCount);
    out.MaterialIndex = prim.material;
    out.NodeIndex = nodeIndex;
    out.SkinIndex = -1;
    out.VertexType = EVertex::E_Static;

    for (size_t v = 0; v < vertexCount; ++v) {
        Vertex vert{};
        vert.Position = ReadVec3Float(src, posAcc, v);
        vert.Normal = normalAcc ? ReadVec3Float(src, *normalAcc, v) : Vec3(0.f, 1.f, 0.f);
        vert.TexCoord = uvAcc ? ReadVec2Float(src, *uvAcc, v) : Vec2(0.f, 0.f);
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
        for (uint32_t i = 0; i < static_cast<uint32_t>(vertexCount); ++i)
            out.Indices[i] = i;
    }

    dst.Mesh.Primitives.push_back(std::move(out));
}

// -----------------------------------------------------------------------------
// Skeletal Mesh primitive 변환
// -----------------------------------------------------------------------------
void GltfLoader::ConvertPrimitive(
    const tinygltf::Model& src
    , const tinygltf::Primitive& prim
    , int nodeIndex
    , int skinIndex
    , GltfLoadResultCPU& dst) {
    if (prim.mode != TINYGLTF_MODE_TRIANGLES)
        return;

    if (skinIndex < 0 || !IsSkinnedPrimitive(prim)) {
        ConvertPrimitive(src, prim, nodeIndex, dst);
        return;
    }

    auto itPos = prim.attributes.find("POSITION");
    if (itPos == prim.attributes.end())
        throw std::runtime_error("Primitive has no POSITION");

    const tinygltf::Accessor& posAcc = src.accessors.at(itPos->second);
    const size_t vertexCount = posAcc.count;

    const tinygltf::Accessor* normalAcc = nullptr;
    const tinygltf::Accessor* uvAcc = nullptr;
    const tinygltf::Accessor* jointsAcc = nullptr;
    const tinygltf::Accessor* weightsAcc = nullptr;

    if (auto it = prim.attributes.find("NORMAL"); it != prim.attributes.end())
        normalAcc = &src.accessors.at(it->second);
    if (auto it = prim.attributes.find("TEXCOORD_0"); it != prim.attributes.end())
        uvAcc = &src.accessors.at(it->second);
    if (auto it = prim.attributes.find("JOINTS_0"); it != prim.attributes.end())
        jointsAcc = &src.accessors.at(it->second);
    if (auto it = prim.attributes.find("WEIGHTS_0"); it != prim.attributes.end())
        weightsAcc = &src.accessors.at(it->second);

    MeshPrimitiveCPU out{};
    out.Vertices.resize(vertexCount);
    out.JointIndices.resize(vertexCount, { 0, 0, 0, 0 });
    out.JointWeights.resize(vertexCount, Vec4(0.f, 0.f, 0.f, 0.f));
    out.MaterialIndex = prim.material;
    out.NodeIndex = nodeIndex;
    out.SkinIndex = skinIndex;
    out.VertexType = EVertex::E_Skinned;

    for (size_t v = 0; v < vertexCount; ++v) {
        Vertex vert{};
        vert.Position = ReadVec3Float(src, posAcc, v);
        vert.Normal = normalAcc ? ReadVec3Float(src, *normalAcc, v) : Vec3(0.f, 1.f, 0.f);
        vert.TexCoord = uvAcc ? ReadVec2Float(src, *uvAcc, v) : Vec2(0.f, 0.f);
        vert.TexCoord.y = 1.f - vert.TexCoord.y;

        std::array<std::uint16_t, 4> joints = ReadJoints4(src, *jointsAcc, v);
        Vec4 weights = ReadVec4Float(src, *weightsAcc, v);

        const float sum = weights.x + weights.y + weights.z + weights.w;
        if (sum > 0.000001f)
            weights /= sum;

        out.JointIndices[v] = joints;
        out.JointWeights[v] = weights;
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
        for (uint32_t i = 0; i < static_cast<uint32_t>(vertexCount); ++i)
            out.Indices[i] = i;
    }

    dst.Mesh.Primitives.push_back(std::move(out));
}

int GltfLoader::TraverseNode(
    const tinygltf::Model& src
    , int srcNodeIndex
    , int parentDstNodeIndex
    , GltfLoadResultCPU& dst) {
    const tinygltf::Node& srcNode = src.nodes.at(srcNodeIndex);

    NodeCPU node{};
    node.Name = srcNode.name;
    node.Parent = parentDstNodeIndex;
    node.MeshIndex = srcNode.mesh;
    node.SkinIndex = srcNode.skin;

    if (srcNode.translation.size() == 3) {
        node.Translation = Vec3(
            (float)srcNode.translation[0],
            (float)srcNode.translation[1],
            (float)srcNode.translation[2]);
    }

    if (srcNode.rotation.size() == 4) {
        node.Rotation = Vec4(
            (float)srcNode.rotation[0],
            (float)srcNode.rotation[1],
            (float)srcNode.rotation[2],
            (float)srcNode.rotation[3]);
    }

    if (srcNode.scale.size() == 3) {
        node.Scale = Vec3(
            (float)srcNode.scale[0],
            (float)srcNode.scale[1],
            (float)srcNode.scale[2]);
    }

    if (srcNode.matrix.size() == 16) {
        node.HasMatrix = true;
        node.LocalMatrix = Mat4(
            (float)srcNode.matrix[0], (float)srcNode.matrix[1], (float)srcNode.matrix[2], (float)srcNode.matrix[3],
            (float)srcNode.matrix[4], (float)srcNode.matrix[5], (float)srcNode.matrix[6], (float)srcNode.matrix[7],
            (float)srcNode.matrix[8], (float)srcNode.matrix[9], (float)srcNode.matrix[10], (float)srcNode.matrix[11],
            (float)srcNode.matrix[12], (float)srcNode.matrix[13], (float)srcNode.matrix[14], (float)srcNode.matrix[15]);
    }

    const int dstNodeIndex = static_cast<int>(dst.Skeleton.Nodes.size());
    dst.Skeleton.Nodes.push_back(std::move(node));
    dst.NodeRemap[srcNodeIndex] = dstNodeIndex;

    if (parentDstNodeIndex >= 0)
        dst.Skeleton.Nodes[parentDstNodeIndex].Children.push_back(dstNodeIndex);

    if (srcNode.mesh >= 0) {
        const tinygltf::Mesh& mesh = src.meshes.at(srcNode.mesh);

        for (const tinygltf::Primitive& prim : mesh.primitives) {
            const bool isSkinned = (srcNode.skin >= 0) && IsSkinnedPrimitive(prim);

            if (isSkinned)
                ConvertPrimitive(src, prim, dstNodeIndex, srcNode.skin, dst);
            else
                ConvertPrimitive(src, prim, dstNodeIndex, dst);
        }
    }

    for (const int childSrcNodeIndex : srcNode.children)
        TraverseNode(src, childSrcNodeIndex, dstNodeIndex, dst);

    return dstNodeIndex;
}

int GltfLoader::FindBestSkinForAnimation(
    const AnimationCPU& animation
    , const GltfSkeletonCPU& skeleton) {
    int bestSkin = -1;
    size_t bestScore = 0;

    for (int skinIndex = 0; skinIndex < static_cast<int>(skeleton.Skins.size()); ++skinIndex) {
        const auto& skin = skeleton.Skins[skinIndex];

        size_t score = 0;
        for (const auto& ch : animation.Channels) {
            if (std::find(skin.Joints.begin(), skin.Joints.end(), ch.TargetNodeIndex) != skin.Joints.end())
                ++score;
        }

        if (score > bestScore) {
            bestScore = score;
            bestSkin = skinIndex;
        }
    }

    return bestSkin;
}

AnimationPathCPU GltfLoader::ToAnimationPath(const std::string& path) {
    if (path == "translation") return AnimationPathCPU::Translation;
    if (path == "rotation")    return AnimationPathCPU::Rotation;
    if (path == "scale")       return AnimationPathCPU::Scale;
    if (path == "weights")     return AnimationPathCPU::Weights;
    return AnimationPathCPU::Unknown;
}

InterpolationCPU GltfLoader::ToInterpolation(const std::string& interpolation) {
    if (interpolation == "STEP") return InterpolationCPU::Step;
    if (interpolation == "CUBICSPLINE") return InterpolationCPU::CubicSpline;
    return InterpolationCPU::Linear;
}