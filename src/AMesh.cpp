#include "pch.h"
#include "AMesh.hpp"

#include "FrankLuna/GeometryGenerator.h"

#include "EditorManager.hpp"
#include RENDERER_HEADER

namespace {
	void ComputeTangents(std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices) {
		// (tangent accumulation, bitangent accumulation)
		std::vector<std::pair<Vec3, Vec3>> temp(vertices.size(), { Vec3(0.f), Vec3(0.f) });

		for (size_t i = 0; i + 2 < indices.size(); i += 3) {
			Vertex& v0 = vertices[indices[i + 0]];
			Vertex& v1 = vertices[indices[i + 1]];
			Vertex& v2 = vertices[indices[i + 2]];

			Vec3 p0 = v0.Position;
			Vec3 p1 = v1.Position;
			Vec3 p2 = v2.Position;

			Vec2 uv0 = v0.TexCoord;
			Vec2 uv1 = v1.TexCoord;
			Vec2 uv2 = v2.TexCoord;

			Vec3 e1 = p1 - p0;
			Vec3 e2 = p2 - p0;

			Vec2 dUV1 = uv1 - uv0;
			Vec2 dUV2 = uv2 - uv0;

			float det = dUV1.x * dUV2.y - dUV1.y * dUV2.x;
			if (fabs(det) < 1e-8f) continue;

			float r = 1.f / det;

			Vec3 tangent = (e1 * dUV2.y - e2 * dUV1.y) * r;
			Vec3 bitangent = (e2 * dUV1.x - e1 * dUV2.x) * r;

			temp[indices[i + 0]].first += tangent;
			temp[indices[i + 1]].first += tangent;
			temp[indices[i + 2]].first += tangent;

			temp[indices[i + 0]].second += bitangent;
			temp[indices[i + 1]].second += bitangent;
			temp[indices[i + 2]].second += bitangent;
		}

		for (size_t i = 0, end = vertices.size(); i < end; ++i) {
			Vertex& v = vertices[i];

			Vec3 N = v.Normal;
			N.Normalize();

			Vec3 T = temp[i].first;
			if (T.LengthSquared() < 1e-8f) {
				// fallback tangent
				Vec3 up = (fabs(N.y) < 0.999f) ? Vec3(0.f, 1.f, 0.f) : Vec3(1.f, 0.f, 0.f);

				Vec3 cross = up.Cross(N);
				T = cross;
				T.Normalize();

				v.Tangent = Vec4(T.x, T.y, T.z, 1.0f);

				continue;
			}

			T = T - N * N.Dot(T);
			T.Normalize();

			Vec3 B = N;
			B.Cross(T);

			float handedness = (B.Dot(temp[i].second) < 0.f) ? -1.f : 1.f;

			v.Tangent = Vec4(T.x, T.y, T.z, handedness);
		}
	}

	void ComputeTangents(std::vector<SkinnedVertex>& vertices, const std::vector<std::uint32_t>& indices) {
		// (tangent accumulation, bitangent accumulation)
		std::vector<std::pair<Vec3, Vec3>> temp(vertices.size(), { Vec3(0.f), Vec3(0.f) });

		for (size_t i = 0; i + 2 < indices.size(); i += 3) {
			SkinnedVertex& v0 = vertices[indices[i + 0]];
			SkinnedVertex& v1 = vertices[indices[i + 1]];
			SkinnedVertex& v2 = vertices[indices[i + 2]];

			Vec3 p0 = v0.Position;
			Vec3 p1 = v1.Position;
			Vec3 p2 = v2.Position;

			Vec2 uv0 = v0.TexCoord;
			Vec2 uv1 = v1.TexCoord;
			Vec2 uv2 = v2.TexCoord;

			Vec3 e1 = p1 - p0;
			Vec3 e2 = p2 - p0;

			Vec2 dUV1 = uv1 - uv0;
			Vec2 dUV2 = uv2 - uv0;

			float det = dUV1.x * dUV2.y - dUV1.y * dUV2.x;
			if (fabs(det) < 1e-8f) continue;

			float r = 1.f / det;

			Vec3 tangent = (e1 * dUV2.y - e2 * dUV1.y) * r;
			Vec3 bitangent = (e2 * dUV1.x - e1 * dUV2.x) * r;

			temp[indices[i + 0]].first += tangent;
			temp[indices[i + 1]].first += tangent;
			temp[indices[i + 2]].first += tangent;

			temp[indices[i + 0]].second += bitangent;
			temp[indices[i + 1]].second += bitangent;
			temp[indices[i + 2]].second += bitangent;
		}

		for (size_t i = 0, end = vertices.size(); i < end; ++i) {
			SkinnedVertex& v = vertices[i];

			Vec3 N = v.Normal;
			N.Normalize();

			Vec3 T = temp[i].first;
			if (T.LengthSquared() < 1e-8f) {
				// fallback tangent
				Vec3 up = (fabs(N.y) < 0.999f) ? Vec3(0.f, 1.f, 0.f) : Vec3(1.f, 0.f, 0.f);

				Vec3 cross = up.Cross(N);
				T = cross;
				T.Normalize();

				v.Tangent = Vec4(T.x, T.y, T.z, 1.0f);

				continue;
			}

			T = T - N * N.Dot(T);
			T.Normalize();

			Vec3 B = N;
			B.Cross(T);

			float handedness = (B.Dot(temp[i].second) < 0.f) ? -1.f : 1.f;

			v.Tangent = Vec4(T.x, T.y, T.z, handedness);
		}
	}

	void BuildMesh(
		const std::vector<GeometryGenerator::Vertex>& vertices
		, const std::vector<std::uint32_t>& indices
		, std::vector<Vertex>& outVertices
		, std::vector<UINT>& outIndices
		, std::vector<Primitive>& outPrimitives
		, AABB& outAABB) {
		Vec3 minPt{ FLT_MAX, FLT_MAX, FLT_MAX };
		Vec3 maxPt{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

		for (size_t i = 0; i < vertices.size(); ++i) {
			const auto& v = vertices[i];

			Vertex vertex{
				Vec3{ v.Position.x, v.Position.y, v.Position.z },
				Vec3{ v.Normal.x, v.Normal.y, v.Normal.z },
				Vec4{ 0.f, 0.f, 0.f, 0.f },
				Vec2{ v.TexC.x, v.TexC.y }
			};

			minPt.x = std::min(minPt.x, vertex.Position.x);
			minPt.y = std::min(minPt.y, vertex.Position.y);
			minPt.z = std::min(minPt.z, vertex.Position.z);

			maxPt.x = std::max(maxPt.x, vertex.Position.x);
			maxPt.y = std::max(maxPt.y, vertex.Position.y);	
			maxPt.z = std::max(maxPt.z, vertex.Position.z);

			outVertices.push_back(vertex);
		}

		outIndices = indices;

        outPrimitives.push_back(Primitive{
			0, static_cast<UINT>(vertices.size()),
			0, static_cast<UINT>(indices.size())});

		Vec3 center = (minPt + maxPt) * 0.5f;
		Vec3 extents = (maxPt - minPt) * 0.5f;

		outAABB = AABB(center, extents);

		ComputeTangents(outVertices, outIndices);
	}

	SkinnedVertex ToSkinnedVertex(
		const Vertex& v
		, const std::array<uint16_t, 4>& joints
		, const Vec4& weights) {
		SkinnedVertex sv{};
		sv.Position = v.Position;
		sv.Normal = v.Normal;
		sv.Tangent = v.Tangent;
		sv.TexCoord = v.TexCoord;

		sv.JointIndices = Uint4(joints[0], joints[1], joints[2], joints[3]);
		sv.JointWeights = weights;

		return sv;
	}
}

AMesh::AMesh()
	: Asset{ EAsset::E_Mesh } {}

AMesh::~AMesh() {}

bool AMesh::Load(const std::wstring& filePath) {
	GltfLoadResultCPU gltf{};
	CheckReturn(GltfLoader::LoadGltfCpu(WStrToStr(filePath), gltf));

	const auto& mesh = gltf.Mesh;

	LOG_INFO(WStrToStr(filePath));
	LOG_INFO(std::format("Loaded model with {} primitives, {} textures, and {} materials.",
		mesh.Primitives.size(),
		mesh.Textures.size(),
		mesh.Materials.size()));

	Vec3 minPt{ FLT_MAX, FLT_MAX, FLT_MAX };
	Vec3 maxPt{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

	for (const auto& primitive : mesh.Primitives) {
		if (primitive.VertexType == EVertex::E_Static) {
			const UINT baseVertex = static_cast<UINT>(mStaticVertices.size());
			const UINT startIndex = static_cast<UINT>(mStaticIndices.size());

			for (const auto& v : primitive.Vertices) {
				mStaticVertices.push_back(v);
				minPt = Vec3::Min(minPt, v.Position);
				maxPt = Vec3::Max(maxPt, v.Position);
			}

			for (const auto& idx : primitive.Indices)
				mStaticIndices.push_back(idx + baseVertex);

			mStaticPrimitives.push_back({
				0,
				static_cast<UINT>(primitive.Vertices.size()),
				startIndex,
				static_cast<UINT>(primitive.Indices.size())
				});
		}
		else {
			const UINT baseVertex = static_cast<UINT>(mSkinnedVertices.size());
			const UINT startIndex = static_cast<UINT>(mSkinnedIndices.size());

			const size_t vertexCount = primitive.Vertices.size();

			for (size_t i = 0; i < vertexCount; ++i) {
				const auto& v = primitive.Vertices[i];

				std::array<uint16_t, 4> joints = { 0, 0, 0, 0 };
				Vec4 weights = Vec4(0, 0, 0, 0);

				if (i < primitive.JointIndices.size())
					joints = primitive.JointIndices[i];

				if (i < primitive.JointWeights.size())
					weights = primitive.JointWeights[i];

				mSkinnedVertices.push_back(ToSkinnedVertex(v, joints, weights));

				minPt = Vec3::Min(minPt, v.Position);
				maxPt = Vec3::Max(maxPt, v.Position);
			}

			for (const auto& idx : primitive.Indices)
				mSkinnedIndices.push_back(idx + baseVertex);

			mSkinnedPrimitives.push_back({
				0,
				static_cast<UINT>(vertexCount),
				startIndex,
				static_cast<UINT>(primitive.Indices.size())
				});
		}
	}

	Vec3 center = (minPt + maxPt) * 0.5f;
	Vec3 extents = (maxPt - minPt) * 0.5f;

	mAABB = AABB(center, extents);

	ComputeTangents(mStaticVertices, mStaticIndices);
	ComputeTangents(mSkinnedVertices, mSkinnedIndices);

	return true;
}

bool AMesh::BuildFromGltf(const std::wstring& filePath, const GltfMeshCPU& mesh) {
	LOG_INFO(std::format("Loaded mesh asset '{}' with {} primitives, {} textures, and {} materials.",
		WStrToStr(filePath),
		mesh.Primitives.size(),
		mesh.Textures.size(),
		mesh.Materials.size()));

	Vec3 minPt{ FLT_MAX, FLT_MAX, FLT_MAX };
	Vec3 maxPt{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

	for (const auto& primitive : mesh.Primitives) {
		if (primitive.VertexType == EVertex::E_Static) {
			const UINT baseVertex = static_cast<UINT>(mStaticVertices.size());
			const UINT startIndex = static_cast<UINT>(mStaticIndices.size());

			for (const auto& v : primitive.Vertices) {
				mStaticVertices.push_back(v);
				minPt = Vec3::Min(minPt, v.Position);
				maxPt = Vec3::Max(maxPt, v.Position);
			}

			for (const auto& idx : primitive.Indices)
				mStaticIndices.push_back(idx + baseVertex);

			mStaticPrimitives.push_back({
				0,
				static_cast<UINT>(primitive.Vertices.size()),
				startIndex,
				static_cast<UINT>(primitive.Indices.size())
				});
		}
		else {
			const UINT baseVertex = static_cast<UINT>(mSkinnedVertices.size());
			const UINT startIndex = static_cast<UINT>(mSkinnedIndices.size());

			const size_t vertexCount = primitive.Vertices.size();

			for (size_t i = 0; i < vertexCount; ++i) {
				const auto& v = primitive.Vertices[i];

				std::array<uint16_t, 4> joints = { 0, 0, 0, 0 };
				Vec4 weights = Vec4(0, 0, 0, 0);

				if (i < primitive.JointIndices.size())
					joints = primitive.JointIndices[i];

				if (i < primitive.JointWeights.size())
					weights = primitive.JointWeights[i];

				mSkinnedVertices.push_back(ToSkinnedVertex(v, joints, weights));

				minPt = Vec3::Min(minPt, v.Position);
				maxPt = Vec3::Max(maxPt, v.Position);
			}

			for (const auto& idx : primitive.Indices)
				mSkinnedIndices.push_back(idx + baseVertex);

			mSkinnedPrimitives.push_back({
				0,
				static_cast<UINT>(vertexCount),
				startIndex,
				static_cast<UINT>(primitive.Indices.size())
				});
		}
	}

	Vec3 center = (minPt + maxPt) * 0.5f;
	Vec3 extents = (maxPt - minPt) * 0.5f;

	mAABB = AABB(center, extents);

	ComputeTangents(mStaticVertices, mStaticIndices);
	ComputeTangents(mSkinnedVertices, mSkinnedIndices);

	return true;
}

bool AMesh::OnAdded() {
	CheckReturn(RENDERER->AddMesh(GetKey(), this));

	return true;
}

bool AMesh::CreateBox() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.f, 1.f, 1.f, 1);
	
	BuildMesh(box.Vertices, box.Indices32, mStaticVertices, mStaticIndices, mStaticPrimitives, mAABB);

	return true;
}

bool AMesh::CreateSphere() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(1.f, 32, 32);

	BuildMesh(sphere.Vertices, sphere.Indices32, mStaticVertices, mStaticIndices, mStaticPrimitives, mAABB);

	return true;
}

bool AMesh::CreatePlane() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData plane = geoGen.CreatePlane(1.f, 1.f, 2, 2);

	BuildMesh(plane.Vertices, plane.Indices32, mStaticVertices, mStaticIndices, mStaticPrimitives, mAABB);

	return true;
}

bool AMesh::CreateCylinder() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(1.f, 1.f, 1.f, 32, 32);
	
	BuildMesh(cylinder.Vertices, cylinder.Indices32, mStaticVertices, mStaticIndices, mStaticPrimitives, mAABB);

	return true;
}

bool AMesh::CreatePyramid() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData pyramid = geoGen.CreatePyramid(1.f, 1.f, 1.f);

	BuildMesh(pyramid.Vertices, pyramid.Indices32, mStaticVertices, mStaticIndices, mStaticPrimitives, mAABB);

	return true;
}

bool AMesh::CreateTorus() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData torus = geoGen.CreateTorus(1.f, 0.5f, 128, 128);

	BuildMesh(torus.Vertices, torus.Indices32, mStaticVertices, mStaticIndices, mStaticPrimitives, mAABB);

	return true;
}

bool AMesh::CreatePrism() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData prism = geoGen.CreatePrism(1.f, 1.f, 8);

	BuildMesh(prism.Vertices, prism.Indices32, mStaticVertices, mStaticIndices, mStaticPrimitives, mAABB);

	return true;
}

bool AMesh::CreateHemisphere() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData hemiSphere = geoGen.CreateHemisphere(1.f, 32, 32);

	BuildMesh(hemiSphere.Vertices, hemiSphere.Indices32, mStaticVertices, mStaticIndices, mStaticPrimitives, mAABB);

	return true;
}

bool AMesh::CreateCapsule() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData capsule = geoGen.CreateCapsule(1.f, 3.f, 32, 32);

	BuildMesh(capsule.Vertices, capsule.Indices32, mStaticVertices, mStaticIndices, mStaticPrimitives, mAABB);

	return true;
}

bool AMesh::CreateTetrahedron() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData tetrahedron = geoGen.CreateTetrahedron(1.f);

	BuildMesh(tetrahedron.Vertices, tetrahedron.Indices32, mStaticVertices, mStaticIndices, mStaticPrimitives, mAABB);

	return true;
}

bool AMesh::CreateOctahedron() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData octahedron = geoGen.CreateOctahedron(1.f);

	BuildMesh(octahedron.Vertices, octahedron.Indices32, mStaticVertices, mStaticIndices, mStaticPrimitives, mAABB);

	return true;
}

bool AMesh::CreateIcosahedron() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData icosahedron = geoGen.CreateIcosahedron(1.f);

	BuildMesh(icosahedron.Vertices, icosahedron.Indices32, mStaticVertices, mStaticIndices, mStaticPrimitives, mAABB);

	return true;
}