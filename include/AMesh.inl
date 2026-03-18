#ifndef __AMESH_INL__
#define __AMESH_INL__

constexpr UINT AMesh::VertexCount() const noexcept {
	return mVertices.size();
}

constexpr UINT AMesh::VerticesByteSize() const noexcept {
	return static_cast<UINT>(mVertices.size() * sizeof(Vertex));
}

constexpr const Vertex* AMesh::Vertices() const noexcept {
	return mVertices.data();
}

constexpr UINT AMesh::IndicesByteSize() const noexcept {
	return static_cast<UINT>(mIndices.size() * sizeof(UINT));
}

constexpr UINT AMesh::IndexCount() const noexcept {
	return mIndices.size();
}

constexpr const UINT* AMesh::Indices() const noexcept {
	return mIndices.data();
}

#endif // __AMESH_INL__