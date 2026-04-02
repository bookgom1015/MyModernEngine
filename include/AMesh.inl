#ifndef __AMESH_INL__
#define __AMESH_INL__

constexpr UINT AMesh::GetStaticVertexCount() const noexcept {
	return static_cast<UINT>(mStaticVertices.size());
}

constexpr UINT AMesh::GetStaticVerticesByteSize() const noexcept {
	return static_cast<UINT>(mStaticVertices.size() * sizeof(Vertex));
}

constexpr const Vertex* AMesh::GetStaticVertices() const noexcept {
	return mStaticVertices.data();
}

constexpr UINT AMesh::GetStaticIndicesByteSize() const noexcept {
	return static_cast<UINT>(mStaticIndices.size() * sizeof(UINT));
}

constexpr UINT AMesh::GetStaticIndexCount() const noexcept {
	return static_cast<UINT>(mStaticIndices.size());
}

constexpr const UINT* AMesh::GetStaticIndices() const noexcept {
	return mStaticIndices.data();
}

constexpr UINT AMesh::GetSkinnedVertexCount() const noexcept {
	return static_cast<UINT>(mSkinnedVertices.size());
}

constexpr UINT AMesh::GetSkinnedVerticesByteSize() const noexcept {
	return static_cast<UINT>(mSkinnedVertices.size() * sizeof(SkinnedVertex));
}

constexpr const SkinnedVertex* AMesh::GetSkinnedVertices() const noexcept {
	return mSkinnedVertices.data();
}

constexpr UINT AMesh::GetSkinnedIndexCount() const noexcept {
	return static_cast<UINT>(mSkinnedIndices.size());
}

constexpr UINT AMesh::GetSkinnedIndicesByteSize() const noexcept {
	return static_cast<UINT>(mSkinnedIndices.size() * sizeof(UINT));
}

constexpr const UINT* AMesh::GetSkinnedIndices() const noexcept {
	return mSkinnedIndices.data();
}

constexpr UINT AMesh::GetStaticPrimitiveCount() const noexcept {
	return static_cast<UINT>(mStaticPrimitives.size());
}

const std::vector<Primitive>& AMesh::GetStaticPrimitives() const noexcept {
	return mStaticPrimitives;
}

constexpr UINT AMesh::GetSkinnedPrimitiveCount() const noexcept {
	return static_cast<UINT>(mSkinnedPrimitives.size());
}

const std::vector<Primitive>& AMesh::GetSkinnedPrimitives() const noexcept {
	return mSkinnedPrimitives;
}

const std::vector<MeshPrimitiveCPU>& AMesh::GetMeshPrimitives() const noexcept {
	return mMeshPrimitives;
}

#endif // __AMESH_INL__