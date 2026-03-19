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

				Hash texc = HashCombine(0, static_cast<Hash>(vert.TexCoord.x));
				texc = HashCombine(texc, static_cast<Hash>(vert.TexCoord.y));

				return HashCombine(HashCombine(pos, normal), texc);
			}
		};
	}
#endif // !__VERTEX_H__