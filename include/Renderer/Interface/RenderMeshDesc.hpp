#pragma once

#include "Renderer/Interface/RenderVertex.hpp"

struct RenderPrimitive {
	std::uint32_t BaseVertexLocation;
	std::uint32_t VertexCount;

	std::uint32_t StartIndexLocation;
	std::uint32_t IndexCount;
};

struct RenderMeshDesc {
	std::vector<RenderVertex> StaticVertices;
	std::vector<std::uint32_t> StaticIndices;
	std::vector<RenderPrimitive> StaticPrimitives;

	std::vector<RenderSkinnedVertex> SkinnedVertices;
	std::vector<std::uint32_t> SkinnedIndices;
	std::vector<RenderPrimitive> SkinnedPrimitives;
};