#include "Renderer/D3D12/pch.h"
#include "Renderer/D3D12/D3D12Renderer.hpp"
#include <cassert>

class D3D12RendererTests {
public:
static int Run() {
 static_assert(sizeof(RenderVertex) == 48);
 static_assert(sizeof(RenderSkinnedVertex) == 80);
 D3D12Renderer renderer;
 renderer.mbCleanedUp = true; // No GPU is initialized in this CPU-only test.
 auto mesh = std::make_unique<D3D12MeshData>();
 mesh->Primitives.push_back({3, 4, 5, 6});
 renderer.mStaticMeshes.emplace(L"static", std::move(mesh));
 auto skinnedMesh = std::make_unique<D3D12MeshData>();
 skinnedMesh->Primitives.push_back({7, 8, 9, 10});
 renderer.mSkinnedMeshes.emplace(L"skinned", std::move(skinnedMesh));
 auto texture = std::make_unique<D3D12Texture>();
 auto* texturePtr = texture.get();
 renderer.mTextures.emplace(L"albedo", std::move(texture));
 RenderObjectDesc rigid;
 rigid.StaticMeshKey = L"static";
 rigid.StaticPrimitiveIndex = 0;
 rigid.World._41 = 12.f;
 rigid.PrevWorld._41 = 11.f;
 rigid.Material.Albedo = Vec3(0.2f, 0.4f, 0.6f);
 rigid.Material.Roughness = 0.3f;
 rigid.Material.Metalness = 0.7f;
 rigid.Material.Specular = 0.8f;
 rigid.Material.AlbedoMapKey = L"albedo";
 rigid.Material.NormalMapKey = L"missing";
 RenderObjectDesc skin;
 skin.SkinnedMeshKey = L"skinned";
 skin.SkinnedPrimitiveIndex = 0;
 skin.SkinIndex = 0;
 skin.BonePalette = {Identity4x4};
 skin.BonePalette[0]._41 = 21.f;
 skin.PrevBonePalette = {Identity4x4};
 skin.PrevBonePalette[0]._41 = 20.f;
 renderer.UpdateRenderObjects(ERenderObjectDomain::E_Opaque, {rigid, skin});
 auto sky = rigid;
 sky.EnvironmentMapKey = L"albedo";
 renderer.UpdateRenderObjects(ERenderObjectDomain::E_SkySphere, {sky});
 assert(renderer.BuildRenderItems());
 assert(renderer.mObjectCBCount == 3);
 const auto& item = *renderer.mRenderItems[D3D12Renderer::E_Static][0];
 assert(item.World._41 == 12.f && item.PrevWorld._41 == 11.f);
 assert(item.AlbedoMap == texturePtr && item.NormalMap == nullptr);
 assert(item.IndexCount == 6 && item.StartIndexLocation == 5 && item.BaseVertexLocation == 3);
 assert(renderer.mMaterials[0].Roughness == 0.3f && renderer.mMaterials[0].Metalness == 0.7f);
 assert(renderer.mRenderItems[D3D12Renderer::E_SkySphere][0]->EnvironmentMap == texturePtr);
 assert(renderer.mCurrentFrameBonePalette[0]._41 == 21.f);
 assert(renderer.mPreviousFrameBonePalette[0]._41 == 20.f);
 assert(renderer.mTextures.size() == 1); // Missing maps never insert null resources.
 auto skin2 = skin;
 skin2.BonePalette[0]._41 = 31.f;
 skin2.PrevBonePalette[0]._41 = 30.f;
 renderer.UpdateRenderObjects(ERenderObjectDomain::E_Opaque, {skin2, skin});
 assert(renderer.BuildRenderItems());
 assert(renderer.mPreviousFrameBonePalette[0]._41 == 30.f);
 assert(renderer.mPreviousFrameBonePalette[1]._41 == 20.f);
 assert(renderer.mRenderItems[D3D12Renderer::E_Skinned][1]->BonePaletteOffset == 1);
 skin.PrevBonePalette.clear();
 renderer.UpdateRenderObjects(ERenderObjectDomain::E_Opaque, {skin});
 assert(renderer.BuildRenderItems());
 assert(renderer.mPreviousFrameBonePalette[0]._41 == 21.f);
 // A mesh-only skeletal component must not abort the frame or consume slots.
 auto pendingSkin = skin;
 pendingSkin.BonePalette.clear();
 pendingSkin.PrevBonePalette.clear();
 renderer.UpdateRenderObjects(ERenderObjectDomain::E_Opaque, {pendingSkin, rigid, skin});
 assert(renderer.BuildRenderItems());
 assert(renderer.mRenderItems[D3D12Renderer::E_Static].size() == 1);
 assert(renderer.mRenderItems[D3D12Renderer::E_Skinned].size() == 1);
 assert(renderer.mObjectCBCount == 3); // rigid + ready skin + sky
 assert(renderer.mMaterials.size() == 2);
 assert(renderer.mRenderItems[D3D12Renderer::E_Skinned][0]->ObjectCBIndex == 1);
 assert(renderer.mCurrentFrameBonePalette.size() == 1);
 // An unresolved skin binding is also a temporary non-renderable state.
 pendingSkin = skin;
 pendingSkin.SkinIndex = -1;
 renderer.UpdateRenderObjects(ERenderObjectDomain::E_Opaque, {pendingSkin});
 assert(renderer.BuildRenderItems());
 assert(renderer.mRenderItems[D3D12Renderer::E_Skinned].empty());
 assert(renderer.mMaterials.empty());
 // Once the skeleton supplies a palette, rendering resumes on the next frame.
 renderer.UpdateRenderObjects(ERenderObjectDomain::E_Opaque, {skin});
 assert(renderer.BuildRenderItems());
 assert(renderer.mRenderItems[D3D12Renderer::E_Skinned].size() == 1);
 assert(renderer.mRenderItems[D3D12Renderer::E_Skinned][0]->BonePaletteOffset == 0);
 renderer.ClearRenderObjects();
 assert(renderer.BuildRenderItems());
 assert(renderer.mObjectCBCount == 0 && renderer.mMaterials.empty());
 assert(renderer.mCurrentFrameBonePalette.empty() && renderer.mPreviousFrameBonePalette.empty());
 rigid.StaticPrimitiveIndex = 99;
 renderer.UpdateRenderObjects(ERenderObjectDomain::E_Opaque, {rigid});
 assert(!renderer.BuildRenderItems());
 return 0;
}
};

int main() { return D3D12RendererTests::Run(); }
