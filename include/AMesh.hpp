#pragma once

#include "Asset.hpp"

#include "Vertex.h"

class AMesh : public Asset {
public:
	AMesh();
	virtual ~AMesh();

public:
	virtual bool Load(const std::wstring& filePath) override;

	virtual bool OnAdded() override;

public:
	bool CreateBox();
	bool CreateSphere();
	bool CreatePlane();
	bool CreateCylinder();
	bool CreatePyramid();
	bool CreateTorus();
	bool CreatePrism();
	bool CreateHemisphere();
	bool CreateCapsule();
	bool CreateTetrahedron();
	bool CreateOctahedron();
	bool CreateIcosahedron();

public:
	__forceinline constexpr UINT GetStaticVertexCount() const noexcept;
	__forceinline constexpr UINT GetStaticVerticesByteSize() const noexcept;
	__forceinline constexpr const Vertex* GetStaticVertices() const noexcept;

	__forceinline constexpr UINT GetStaticIndexCount() const noexcept;
	__forceinline constexpr UINT GetStaticIndicesByteSize() const noexcept;
	__forceinline constexpr const UINT* GetStaticIndices() const noexcept;

	__forceinline constexpr UINT GetSkinnedVertexCount() const noexcept;
	__forceinline constexpr UINT GetSkinnedVerticesByteSize() const noexcept;
	__forceinline constexpr const SkinnedVertex* GetSkinnedVertices() const noexcept;

	__forceinline constexpr UINT GetSkinnedIndexCount() const noexcept;
	__forceinline constexpr UINT GetSkinnedIndicesByteSize() const noexcept;
	__forceinline constexpr const UINT* GetSkinnedIndices() const noexcept;

	__forceinline constexpr UINT GetStaticPrimitiveCount() const noexcept;
	__forceinline const std::vector<Primitive>& GetStaticPrimitives() const noexcept;

	__forceinline constexpr UINT GetSkinnedPrimitiveCount() const noexcept;
	__forceinline const std::vector<Primitive>& GetSkinnedPrimitives() const noexcept;

private:
	std::vector<SkinnedVertex> mSkinnedVertices;
	std::vector<UINT> mSkinnedIndices;

	std::vector<Vertex> mStaticVertices;
	std::vector<UINT> mStaticIndices;

	std::vector<Primitive> mStaticPrimitives;
	std::vector<Primitive> mSkinnedPrimitives;

	AABB mAABB;
};

#include "AMesh.inl"