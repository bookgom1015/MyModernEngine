#ifndef __VERTEX_H__
#define __VERTEX_H__

struct Vertex {
	Vec3 Position;
	Vec3 Normal;
	Vec4 Tangent;
	Vec2 TexCoord;

#ifndef _HLSL
	bool operator==(const Vertex& other) const;
#endif
};

struct SkinnedVertex {
	Vec3	Position;
	Vec3	Normal;
	Vec4	Tangent;
	Vec2	TexCoord;
	Uint4	JointIndices;
	Vec4	JointWeights;

#ifndef _HLSL
	bool operator==(const SkinnedVertex& other) const;
#endif
};

#ifdef _HLSL
	#ifndef VERTEX_IN
	#define VERTEX_IN					\
	struct VertexIn {					\
		float3 PosL		: POSITION0;	\
		float3 NormalL	: NORMAL0;		\
		float4 TangentL : TANGENT0;		\
		float2 TexC		: TEXCOORD;		\
	};
	#endif

	#ifndef SKINNED_VERTEX_IN
	#define SKINNED_VERTEX_IN						\
	struct SkinnedVertexIn {						\
		float3	PosL			: POSITION0;		\
		float3	NormalL			: NORMAL0;			\
		float4	TangentL		: TANGENT0;			\
		float2	TexC			: TEXCOORD;			\
		Uint4	JointIndices	: BLENDINDICES0;	\
		float4	JointWeights	: BLENDWEIGHTS0;	\
	};
	#endif
#else
namespace std {
	template<>
	struct hash<Vertex> {
		Hash operator()(const Vertex& vert) const {
			Hash pos = HashCombine(0, static_cast<Hash>(vert.Position.x));
			pos = HashCombine(pos, static_cast<Hash>(vert.Position.y));
			pos = HashCombine(pos, static_cast<Hash>(vert.Position.z));

			Hash normal = HashCombine(0, static_cast<Hash>(vert.Normal.x));
			normal = HashCombine(normal, static_cast<Hash>(vert.Normal.y));
			normal = HashCombine(normal, static_cast<Hash>(vert.Normal.z));

			Hash tangent = HashCombine(0, static_cast<Hash>(vert.Tangent.x));
			tangent = HashCombine(tangent, static_cast<Hash>(vert.Tangent.y));
			tangent = HashCombine(tangent, static_cast<Hash>(vert.Tangent.z));

			Hash texc = HashCombine(0, static_cast<Hash>(vert.TexCoord.x));
			texc = HashCombine(texc, static_cast<Hash>(vert.TexCoord.y));
						
			return HashCombine(pos, HashCombine(normal, HashCombine(texc, tangent)));
		}
	};

	template<>
	struct hash<SkinnedVertex> {
		Hash operator()(const SkinnedVertex& vert) const {
			Hash pos = HashCombine(0, static_cast<Hash>(vert.Position.x));
			pos = HashCombine(pos, static_cast<Hash>(vert.Position.y));
			pos = HashCombine(pos, static_cast<Hash>(vert.Position.z));

			Hash normal = HashCombine(0, static_cast<Hash>(vert.Normal.x));
			normal = HashCombine(normal, static_cast<Hash>(vert.Normal.y));
			normal = HashCombine(normal, static_cast<Hash>(vert.Normal.z));

			Hash tangent = HashCombine(0, static_cast<Hash>(vert.Tangent.x));
			tangent = HashCombine(tangent, static_cast<Hash>(vert.Tangent.y));
			tangent = HashCombine(tangent, static_cast<Hash>(vert.Tangent.z));

			Hash texc = HashCombine(0, static_cast<Hash>(vert.TexCoord.x));
			texc = HashCombine(texc, static_cast<Hash>(vert.TexCoord.y));

			Hash joints = HashCombine(0, vert.JointIndices.x);
			joints = HashCombine(joints, vert.JointIndices.y);
			joints = HashCombine(joints, vert.JointIndices.z);
			joints = HashCombine(joints, vert.JointIndices.w);

			Hash weights = HashCombine(0, static_cast<Hash>(vert.JointWeights.x));
			weights = HashCombine(weights, static_cast<Hash>(vert.JointWeights.y));
			weights = HashCombine(weights, static_cast<Hash>(vert.JointWeights.z));
			weights = HashCombine(weights, static_cast<Hash>(vert.JointWeights.w));

			return HashCombine(pos, HashCombine(normal, HashCombine(texc
				, HashCombine(tangent, HashCombine(joints, weights)))));
		}
	};
}
#endif
	
#endif // !__VERTEX_H__