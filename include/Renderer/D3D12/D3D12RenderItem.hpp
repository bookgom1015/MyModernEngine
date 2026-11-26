#pragma once

struct D3D12MeshData;
struct D3D12MaterialData;
struct D3D12Texture;

struct D3D12RenderItem {
	INT NumFramesDirty;
	
	INT ObjectCBIndex = -1;
	INT MaterialCBIndex = -1;

	UINT BonePaletteOffset = 0;
	int SkinIndex = -1;

	D3D12Texture* AlbedoMap = nullptr;
	D3D12Texture* NormalMap = nullptr;

	D3D12Texture* EnvironmentMap = nullptr;

	D3D12MeshData* MeshData = nullptr; 

	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	Mat4 World = Identity4x4;
	Mat4 PrevWorld = Identity4x4;
	Mat4 TexTransform = Identity4x4;

	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	UINT BaseVertexLocation = 0;
};