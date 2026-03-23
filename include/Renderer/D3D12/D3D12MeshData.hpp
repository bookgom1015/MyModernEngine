#pragma once

struct D3D12MeshData {
	Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU;
	Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader;

	UINT VertexByteStride;
	UINT VertexBufferByteSize;

	DXGI_FORMAT IndexFormat;
	UINT IndexBufferByteSize;
	UINT IndexByteStride;

	UINT64 Fence;

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView() const;
	
	void ReleaseUploadBuffers();
};