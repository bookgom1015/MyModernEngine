#include "pch.h"
#include "AMesh.hpp"

#include "FrankLuna/GeometryGenerator.h"

AMesh::AMesh()
	: Asset{ EAsset::E_Mesh } {}

AMesh::~AMesh() {}

bool AMesh::CreateBox() {
	GeometryGenerator geoGen{};
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.f, 1.f, 1.f, 1);

	for (size_t i = 0; i < box.Vertices.size(); ++i) {
		const auto& v = box.Vertices[i];
		Vertex vertex{
			Vec3{ v.Position.x, v.Position.y, v.Position.z },
			Vec3{ v.Normal.x, v.Normal.y, v.Normal.z },
			Vec2{ v.TexC.x, v.TexC.y }
		};
		mVertices.push_back(vertex);
	}
	mIndices = box.Indices32;


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
	GeometryGenerator::MeshData plane = geoGen.CreateQuad(1.f, 1.f, 1.f, 1.f, 1.f);

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