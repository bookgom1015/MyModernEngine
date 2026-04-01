#pragma once

namespace EAsset {
	enum Type {
		E_Mesh,
		E_Material,
		E_Texture,
		E_Sound,
		E_Level,
		E_Sprite,
		E_Skeleton,
		E_AnimationClip,
		E_Prefab,
		Count
	};
}

namespace EComponent {
	enum Type {
		E_CompButton = -1,
		E_Transform = 0,
		E_Camera,
		E_Collider,
		E_Light,
		E_MeshRender,
		E_SkeletalMeshRender,
		E_SpriteRender,
		E_ParticleRender,
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

namespace ELevelLayer {
	enum Type {
		E_Default = 0,
		E_Camera,
		E_Light,
		E_Player,
		E_Enemy,
		E_Ground,
		E_Projectile,
		E_Particle,
		E_Background,
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
		E_ChangeNewLevel,
		E_ChangeLevelState,
		E_DeferredProcessing,
		Count
	};
}

namespace ETrasnformDependency {
	enum Type {
		E_Scale			= 1 << 0,
		E_Rotation		= 1 << 1,
		E_Translation	= 1 << 2,
		E_All			= E_Scale | E_Rotation | E_Translation
	};
}

namespace ETransformDirection {
	enum Type {
		E_Forward,
		E_Up,
		E_Right,
		Count
	};
}

namespace EProjection {
	enum Type {
		E_Perspective,
		E_Orthographic
	};
}

namespace ELight {
	enum Type {
		E_Directional,
		E_Point,
		E_Spot,
		E_Tube,
		E_Rectangle,
		Count
	};
}

namespace EVertex {
	enum Type {
		E_Skinned,
		E_Static,
		Count
	};
}