#pragma once

namespace EAsset {
	enum Type {
		E_Mesh,
		E_Material,
		E_Texture,
		E_Level,
		E_Sprite,
		E_Skeleton,
		E_AnimationClip,
		Count
	};
}

namespace EComponent {
	enum Type {
		E_CompButton = -1,
		E_Transform = 0,
		E_Camera,
		E_Light,
		E_MeshRender,
		E_SkeletalMeshRender,
		E_SkySphereRender,
		E_ReflectionProbe,
		E_Rigidbody,
		E_BoxCollider,
		E_SphereCollider,
		E_CapsuleCollider,
		E_MeshCollider,
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
		E_ReflectionProbe,
		Count
	};
}


namespace ERenderDomain {
	enum Type {
		E_Opaque,
		E_Masked,
		E_Transparent,
		E_SkySphere,
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

namespace EProbeShape {
	enum Type {
		E_Sphere = 0,
		E_Box,
		Count
	};
}

namespace EProbeBakeState {
	enum Type {
		E_Unbaked,
		E_Dirty,
		E_Baking,
		E_Ready,
		E_Failed,
		Count
	};
}

namespace ERigidbody {
	enum Type {
		E_Static,		// 안 움직임. 벽, 바닥
		E_Kinematic,	// 코드로 움직임. 물리 힘 영향 X
		E_Dynamic,		// 힘/중력/충돌에 의해 움직임
		Count
	};
}

namespace ERigidbodyConstraint {
	enum Type {
		E_None = 0,
		E_FreezePositionX = 1 << 0,
		E_FreezePositionY = 1 << 1,
		E_FreezePositionZ = 1 << 2,
		E_FreezeRotationX = 1 << 3,
		E_FreezeRotationY = 1 << 4,
		E_FreezeRotationZ = 1 << 5
	};
}

namespace ECollider {
	enum Type {
		E_Box,
		E_Sphere,
		E_Capsule,
		E_Mesh,
		Count
	};
}

namespace ECapsuleAxis {
	enum Type {
		E_XAxis,
		E_YAxis,
		E_ZAxis
	};
}

namespace EMeshCollision {
	enum Type {
		E_Convex,
		E_Triangle
	};
}