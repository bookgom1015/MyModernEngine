#include "pch.h"
#include "AMesh.hpp"

#include "FrankLuna/GeometryGenerator.h"

#include RENDERER_HEADER

AMesh::AMesh()
	: Asset{ EAsset::E_Mesh } {}

AMesh::~AMesh() {}

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

	for (size_t i = 0; i < sphere.Vertices.size(); ++i) {
		const auto& v = sphere.Vertices[i];
		Vertex vertex{
			Vec3{ v.Position.x, v.Position.y, v.Position.z },
			Vec3{ v.Normal.x, v.Normal.y, v.Normal.z },
			Vec2{ v.TexC.x, v.TexC.y }
		};
		mVertices.push_back(vertex);
	}

	mIndices = sphere.Indices32;

	return true;
}

bool AMesh::CreatePlane() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData plane = geoGen.CreateGrid(1.f, 1.f, 2, 2);

	for (size_t i = 0; i < plane.Vertices.size(); ++i) {
		const auto& v = plane.Vertices[i];
		Vertex vertex{
			Vec3{ v.Position.x, v.Position.y, v.Position.z },
			Vec3{ v.Normal.x, v.Normal.y, v.Normal.z },
			Vec2{ v.TexC.x, v.TexC.y }
		};
		mVertices.push_back(vertex);
	}

	mIndices = plane.Indices32;

	return true;
}

bool AMesh::CreateCylinder() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(1.f, 1.f, 1.f, 32, 32);
	
	for (size_t i = 0; i < cylinder.Vertices.size(); ++i) {
		const auto& v = cylinder.Vertices[i];
		Vertex vertex{
			Vec3{ v.Position.x, v.Position.y, v.Position.z },
			Vec3{ v.Normal.x, v.Normal.y, v.Normal.z },
			Vec2{ v.TexC.x, v.TexC.y }
		};
		mVertices.push_back(vertex);
	}

	mIndices = cylinder.Indices32;

	return true;
}

bool AMesh::CreatePyramid() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData pyramid = geoGen.CreatePyramid(1.f, 1.f, 1.f);

	for (size_t i = 0; i < pyramid.Vertices.size(); ++i) {
		const auto& v = pyramid.Vertices[i];
		Vertex vertex{
			Vec3{ v.Position.x, v.Position.y, v.Position.z },
			Vec3{ v.Normal.x, v.Normal.y, v.Normal.z },
			Vec2{ v.TexC.x, v.TexC.y }
		};
		mVertices.push_back(vertex);
	}

	mIndices = pyramid.Indices32;

	return true;
}

bool AMesh::CreateTorus() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData torus = geoGen.CreateTorus(1.f, 1.f, 128, 128);

	for (size_t i = 0; i < torus.Vertices.size(); ++i) {
		const auto& v = torus.Vertices[i];
		Vertex vertex{
			Vec3{ v.Position.x, v.Position.y, v.Position.z },
			Vec3{ v.Normal.x, v.Normal.y, v.Normal.z },
			Vec2{ v.TexC.x, v.TexC.y }
		};
		mVertices.push_back(vertex);
	}

	mIndices = torus.Indices32;

	return true;
}

bool AMesh::CreatePrism() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData prism = geoGen.CreatePrism(1.f, 1.f, 8);

	for (size_t i = 0; i < prism.Vertices.size(); ++i) {
		const auto& v = prism.Vertices[i];
		Vertex vertex{
			Vec3{ v.Position.x, v.Position.y, v.Position.z },
			Vec3{ v.Normal.x, v.Normal.y, v.Normal.z },
			Vec2{ v.TexC.x, v.TexC.y }
		};
		mVertices.push_back(vertex);
	}

	mIndices = prism.Indices32;

	return true;
}

bool AMesh::CreateHemisphere() {
	return true;
}

bool AMesh::CreateCapsule() {
	return true;
}

bool AMesh::CreateTetrahedron() {
	return true;
}

bool AMesh::CreateOctahedron() {
	return true;
}

bool AMesh::CreateIcosahedron() {
	return true;
}

bool AMesh::RegisterToRenderer() {
	CheckReturn(RENDERER->AddMesh(GetKey(), this));

	return true;
}