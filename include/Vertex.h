#ifndef __VERTEX_H__
#define __VERTEX_H__

struct Vertex {
	Vec3 Position;
	Vec3 Normal;
	Vec2 TexCoord;

#ifndef _HLSL
	bool operator==(const Vertex& other) const;
#endif
};

#ifdef _HLSL
	#ifndef VERTEX_IN
	#define VERTEX_IN
	struct VertexIn {
		float3 PosL		: POSITION0;
		float3 NormalL	: NORMAL0;
		float2 TexC		: TEXCOORD;
	};
	#endif
#endif

#endif // !__VERTEX_H__