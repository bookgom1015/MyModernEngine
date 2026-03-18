#pragma once

#include "Asset.hpp"

#include "Vertex.h"

class AMesh : public Asset {
public:
	AMesh();
	virtual ~AMesh();



public:
	__forceinline constexpr UINT VertexCount() const noexcept;
	__forceinline constexpr UINT VerticesByteSize() const noexcept;
	__forceinline constexpr const Vertex* Vertices() const noexcept;

	__forceinline constexpr UINT IndexCount() const noexcept;
	__forceinline constexpr UINT IndicesByteSize() const noexcept;
	__forceinline constexpr const UINT* Indices() const noexcept;

private:
	std::unordered_map<Vertex, UINT> mUniqueVertices;
	std::vector<Vertex> mVertices;
	std::vector<UINT> mIndices;
};

#include "AMesh.inl"