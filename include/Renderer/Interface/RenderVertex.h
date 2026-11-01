#ifndef __RENDER_VERTEX_H__
#define __RENDER_VERTEX_H__

struct RenderVertex {
	Vec3 Position;
	Vec3 Normal;
	Vec4 Tangent;
	Vec2 TexCoord;
};

struct RenderSkinnedVertex {
	Vec3	Position;
	Vec3	Normal;
	Vec4	Tangent;
	Vec2	TexCoord;
	Uint4	JointIndices;
	Vec4	JointWeights;
};

#endif // __RENDER_VERTEX_H__