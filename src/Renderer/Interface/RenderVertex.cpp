#include "Renderer/Interface/pch.h"
#include "Renderer/Interface/RenderVertex.hpp"

bool operator==(const RenderVertex& lhs, const RenderVertex& rhs) {
	return lhs.Position == rhs.Position
		&& lhs.Normal == rhs.Normal
		&& lhs.Tangent == rhs.Tangent
		&& lhs.TexCoord == rhs.TexCoord;
}

bool operator==(const RenderSkinnedVertex& lhs, const RenderSkinnedVertex& rhs) {
	return lhs.Position == rhs.Position
		&& lhs.Normal == rhs.Normal
		&& lhs.Tangent == rhs.Tangent
		&& lhs.TexCoord == rhs.TexCoord
		&& lhs.JointIndices.x == rhs.JointIndices.x
		&& lhs.JointIndices.y == rhs.JointIndices.y
		&& lhs.JointIndices.z == rhs.JointIndices.z
		&& lhs.JointIndices.w == rhs.JointIndices.w
		&& lhs.JointWeights == rhs.JointWeights;
}