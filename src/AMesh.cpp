#include "pch.h"
#include "AMesh.hpp"

#include "FrankLuna/GeometryGenerator.h"

#include "EditorManager.hpp"
#include RENDERER_HEADER

#include "GltfLoader.hpp"

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
			outVertices.push_back(vertex);
		}

		outIndices = indices;


		outPrimitives.emplace_back(
			0, static_cast<UINT>(vertices.size()),
			0, static_cast<UINT>(indices.size()));

		Vec3 center = (minPt + maxPt) * 0.5f;
		Vec3 extents = (maxPt - minPt) * 0.5f;

		outAABB = AABB(center, extents);

		ComputeTangents(outVertices, outIndices);
	}
}

AMesh::AMesh()
	: Asset{ EAsset::E_Mesh } {}

AMesh::~AMesh() {}

bool AMesh::Load(const std::wstring& filePath) {
	GltfModelCPU model{};
	CheckReturn(GltfLoader::LoadGltfCpu(WStrToStr(filePath), model));

	LOG_INFO(WStrToStr(filePath));
	LOG_INFO(std::format("Loaded model with {} primitives, {} textures, and {} materials.",
		model.Primitives.size(),
		model.Textures.size(),
		model.Materials.size()));

	Vec3 minPt{ FLT_MAX, FLT_MAX, FLT_MAX };
	Vec3 maxPt{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

	for (const auto& primitive : model.Primitives) {
		const auto baseVertex = static_cast<UINT>(mVertices.size());
		const auto vertexCount = static_cast<UINT>(primitive.Vertices.size());
		for (const auto& vertex : primitive.Vertices) {
			mVertices.push_back(vertex);
		}

		const auto startIndex = static_cast<UINT>(mIndices.size());
		const auto indexCount = static_cast<UINT>(primitive.Indices.size());
		for (const auto& index : primitive.Indices) {
			mIndices.push_back(index + baseVertex);
		}

		mPrimitives.emplace_back(
			baseVertex,
			vertexCount,
			startIndex,
			indexCount);

		Vec3 center = (minPt + maxPt) * 0.5f;
		Vec3 extents = (maxPt - minPt) * 0.5f;

		mAABB = AABB(center, extents);

		ComputeTangents(mVertices, mIndices);
	}

	return true;
}

bool AMesh::OnAdded() {
	CheckReturn(RENDERER->AddMesh(GetKey(), this));

	return true;
}

bool AMesh::CreateBox() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.f, 1.f, 1.f, 1);
	
	BuildMesh(box.Vertices, box.Indices32, mVertices, mIndices, mPrimitives, mAABB);

	return true;
}

bool AMesh::CreateSphere() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(1.f, 32, 32);

	BuildMesh(sphere.Vertices, sphere.Indices32, mVertices, mIndices, mPrimitives, mAABB);

	return true;
}

bool AMesh::CreatePlane() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData plane = geoGen.CreatePlane(1.f, 1.f, 2, 2);

	BuildMesh(plane.Vertices, plane.Indices32, mVertices, mIndices, mPrimitives, mAABB);

	return true;
}

bool AMesh::CreateCylinder() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(1.f, 1.f, 1.f, 32, 32);
	
	BuildMesh(cylinder.Vertices, cylinder.Indices32, mVertices, mIndices, mPrimitives, mAABB);

	return true;
}

bool AMesh::CreatePyramid() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData pyramid = geoGen.CreatePyramid(1.f, 1.f, 1.f);

	BuildMesh(pyramid.Vertices, pyramid.Indices32, mVertices, mIndices, mPrimitives, mAABB);

	return true;
}

bool AMesh::CreateTorus() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData torus = geoGen.CreateTorus(1.f, 0.5f, 128, 128);

	BuildMesh(torus.Vertices, torus.Indices32, mVertices, mIndices, mPrimitives, mAABB);

	return true;
}

bool AMesh::CreatePrism() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData prism = geoGen.CreatePrism(1.f, 1.f, 8);

	BuildMesh(prism.Vertices, prism.Indices32, mVertices, mIndices, mPrimitives, mAABB);

	return true;
}

bool AMesh::CreateHemisphere() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData hemiSphere = geoGen.CreateHemisphere(1.f, 32, 32);

	BuildMesh(hemiSphere.Vertices, hemiSphere.Indices32, mVertices, mIndices, mPrimitives, mAABB);

	return true;
}

bool AMesh::CreateCapsule() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData capsule = geoGen.CreateCapsule(1.f, 3.f, 32, 32);

	BuildMesh(capsule.Vertices, capsule.Indices32, mVertices, mIndices, mPrimitives, mAABB);

	return true;
}

bool AMesh::CreateTetrahedron() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData tetrahedron = geoGen.CreateTetrahedron(1.f);

	BuildMesh(tetrahedron.Vertices, tetrahedron.Indices32, mVertices, mIndices, mPrimitives, mAABB);

	return true;
}

bool AMesh::CreateOctahedron() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData octahedron = geoGen.CreateOctahedron(1.f);

	BuildMesh(octahedron.Vertices, octahedron.Indices32, mVertices, mIndices, mPrimitives, mAABB);

	return true;
}

bool AMesh::CreateIcosahedron() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData icosahedron = geoGen.CreateIcosahedron(1.f);

	BuildMesh(icosahedron.Vertices, icosahedron.Indices32, mVertices, mIndices, mPrimitives, mAABB);

	return true;
}