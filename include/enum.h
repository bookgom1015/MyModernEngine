#pragma once

namespace EAsset {
	enum Type {
		E_Mesh,
		E_Material,
		E_Texture,
		E_Sound,
		E_GraphicShader,
		E_ComputeShader,
		E_Level,
		E_Sprite,
		E_Flipbook,
		E_TileMap,
		E_Prefab,
		Count
	};
}

static std::string AssetTypeToString(EAsset::Type type) {
	switch (type) {
	case EAsset::E_Mesh: return "Mesh";
	case EAsset::E_Material: return "Material";
	case EAsset::E_Texture: return "Texture";
	case EAsset::E_Sound: return "Sound";
	case EAsset::E_GraphicShader: return "GraphicShader";
	case EAsset::E_ComputeShader: return "ComputeShader";
	case EAsset::E_Level: return "Level";
	case EAsset::E_Sprite: return "Sprite";
	case EAsset::E_Flipbook: return "Flipbook";
	case EAsset::E_TileMap: return "TileMap";
	case EAsset::E_Prefab: return "Prefab";
	default: return "Unknown";
	}
}

namespace EComponent {
	enum Type {
		E_None = -1,
		E_Transform = 0,
		E_Camera,
		E_Collider2D,
		E_Collider3D,
		E_Light2D,
		E_Light3D,
		E_MeshRender,
		E_BillboardRender,
		E_SpriteRender,
		E_FlipbookRender,
		E_ParticleRender,
		E_TileRender,
		E_Rigidbody,
		Count,
		E_Script
	};
}

namespace ELevelState {
	enum Type {
		E_Playing = 0,
		E_Paused,
		E_Stopped,
		Count
	};
}

namespace ERenderDomain {
	enum Type {
		E_Opaque,
		E_Masked,
		E_Transparent,
		Count
	};
}

namespace ETask {
	enum Type {
		E_CreateObject,
		E_DestroyObject,
		E_ChangeLevel,
		E_ChangeLevelState,
		E_DeferredProcessing,
		Count
	};
}