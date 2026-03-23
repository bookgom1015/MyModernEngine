#include "pch.h"
#include "Renderer/D3D12/D3D12MeshData.hpp"

D3D12_VERTEX_BUFFER_VIEW D3D12MeshData::VertexBufferView() const {
	D3D12_VERTEX_BUFFER_VIEW vbv{};
	vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
	vbv.StrideInBytes = VertexByteStride;
	vbv.SizeInBytes = VertexBufferByteSize;

	return vbv;
}

D3D12_INDEX_BUFFER_VIEW D3D12MeshData::IndexBufferView() const {
	D3D12_INDEX_BUFFER_VIEW ibv{};
	ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
	ibv.Format = IndexFormat;
	ibv.SizeInBytes = IndexBufferByteSize;

	return ibv;
}

void D3D12MeshData::ReleaseUploadBuffers() {
	VertexBufferUploader.Reset();
	IndexBufferUploader.Reset();
}