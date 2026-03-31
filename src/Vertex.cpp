#include "pch.h"
#include "Vertex.h"

bool Vertex::operator==(const Vertex& other) const {
	return Position == other.Position 
		&& Normal == other.Normal
		&& TexCoord == other.TexCoord;
}

bool SkinnedVertex::operator==(const SkinnedVertex& other) const {
	return Position == other.Position
		&& Normal == other.Normal
		&& TexCoord == other.TexCoord
		&& JointIndices.x == other.JointIndices.x
		&& JointIndices.y == other.JointIndices.y
		&& JointIndices.z == other.JointIndices.z
		&& JointIndices.w == other.JointIndices.w
		&& JointWeights == other.JointWeights;
}