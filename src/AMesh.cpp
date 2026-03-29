#include "pch.h"
#include "AMesh.hpp"

#include "FrankLuna/GeometryGenerator.h"

#include "EditorManager.hpp"
#include RENDERER_HEADER

#include "GltfLoader.hpp"

namespace {
	void BuildMesh(
		const std::vector<GeometryGenerator::Vertex>& vertices
		, const std::vector<std::uint32_t>& indices
		, std::vector<Vertex>& outVertices
		, std::vector<UINT>& outIndices) {
		for (size_t i = 0; i < vertices.size(); ++i) {
			const auto& v = vertices[i];
			Vertex vertex{
				Vec3{ v.Position.x, v.Position.y, v.Position.z },
				Vec3{ v.Normal.x, v.Normal.y, v.Normal.z },
				Vec2{ v.TexC.x, v.TexC.y }
			};
			outVertices.push_back(vertex);
		}

		outIndices = indices;
	}
}

AMesh::AMesh()
	: Asset{ EAsset::E_Mesh } {}

AMesh::~AMesh() {}

bool AMesh::Load(const std::wstring& filePath) {
	LOG_INFO(WStrToStr(filePath));

	GltfModelCPU model{};
	CheckReturn(GltfLoader::LoadGltfCpu(WStrToStr(filePath), model));

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
	}

	return true;
}

bool AMesh::OnAdded() {
	CheckReturn(RegisterToRenderer());

	return true;
}

bool AMesh::CreateBox() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.f, 1.f, 1.f, 1);
	
	Vec3 minPt{ FLT_MAX, FLT_MAX, FLT_MAX };
	Vec3 maxPt{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

	for (size_t i = 0; i < box.Vertices.size(); ++i) {
		const auto& v = box.Vertices[i];
		Vertex vertex{
			Vec3{ v.Position.x, v.Position.y, v.Position.z },
			Vec3{ v.Normal.x, v.Normal.y, v.Normal.z },
			Vec2{ v.TexC.x, v.TexC.y }
		};

		minPt = Vec3::Min(minPt, vertex.Position);
		maxPt = Vec3::Max(maxPt, vertex.Position);

		mVertices.push_back(vertex);

	}
	mIndices = box.Indices32;

	Vec3 center = (minPt + maxPt) * 0.5f;
	Vec3 extents = (maxPt - minPt) * 0.5f;

	mAABB = AABB(center, extents);

	return true;
}

bool AMesh::CreateSphere() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(1.f, 32, 32);

	BuildMesh(sphere.Vertices, sphere.Indices32, mVertices, mIndices);

	return true;
}

bool AMesh::CreatePlane() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData plane = geoGen.CreatePlane(1.f, 1.f, 2, 2);

	BuildMesh(plane.Vertices, plane.Indices32, mVertices, mIndices);

	return true;
}

bool AMesh::CreateCylinder() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(1.f, 1.f, 1.f, 32, 32);
	
	BuildMesh(cylinder.Vertices, cylinder.Indices32, mVertices, mIndices);

	return true;
}

bool AMesh::CreatePyramid() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData pyramid = geoGen.CreatePyramid(1.f, 1.f, 1.f);

	BuildMesh(pyramid.Vertices, pyramid.Indices32, mVertices, mIndices);

	return true;
}

bool AMesh::CreateTorus() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData torus = geoGen.CreateTorus(1.f, 0.5f, 128, 128);

	BuildMesh(torus.Vertices, torus.Indices32, mVertices, mIndices);

	return true;
}

bool AMesh::CreatePrism() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData prism = geoGen.CreatePrism(1.f, 1.f, 8);

	BuildMesh(prism.Vertices, prism.Indices32, mVertices, mIndices);

	return true;
}

bool AMesh::CreateHemisphere() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData hemiSphere = geoGen.CreateHemisphere(1.f, 32, 32);

	BuildMesh(hemiSphere.Vertices, hemiSphere.Indices32, mVertices, mIndices);

	return true;
}

bool AMesh::CreateCapsule() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData capsule = geoGen.CreateCapsule(1.f, 3.f, 32, 32);

	BuildMesh(capsule.Vertices, capsule.Indices32, mVertices, mIndices);

	return true;
}

bool AMesh::CreateTetrahedron() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData tetrahedron = geoGen.CreateTetrahedron(1.f);

	BuildMesh(tetrahedron.Vertices, tetrahedron.Indices32, mVertices, mIndices);

	return true;
}

bool AMesh::CreateOctahedron() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData octahedron = geoGen.CreateOctahedron(1.f);

	BuildMesh(octahedron.Vertices, octahedron.Indices32, mVertices, mIndices);

	return true;
}

bool AMesh::CreateIcosahedron() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData icosahedron = geoGen.CreateIcosahedron(1.f);

	BuildMesh(icosahedron.Vertices, icosahedron.Indices32, mVertices, mIndices);

	return true;
}

bool AMesh::RegisterToRenderer() {
	CheckReturn(RENDERER->AddMesh(GetKey(), this));

	return true;
}