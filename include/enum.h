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