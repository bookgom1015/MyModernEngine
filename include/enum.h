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
		E_Sound,
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

#include "LightData.h"

namespace EVertex {
	enum Type {
		E_Skinned,
		E_Static,
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

	static const char* TypeNames[] = { "Static", "Kinematic", "Dynamic" };
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



namespace EAudioChannel {
	enum Type {
		E_Master,
		E_Editor,
		E_Music,
		E_SFX,
		E_Voice,
		E_CommonChannel0,
		E_CommonChannel1,
		E_CommonChannel2,
		E_CommonChannel3,
		E_CommonChannel4,
		E_CommonChannel5,
		E_CommonChannel6,
		E_CommonChannel7,
		E_CommonChannel8,
		E_CommonChannel9,
		E_CommonChannel10,
		E_CommonChannel11,
		Count
	};
}