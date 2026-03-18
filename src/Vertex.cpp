#include "pch.h"
#include "Vertex.h"

bool Vertex::operator==(const Vertex& other) const {
	return Position == other.Position 
		&& Normal == other.Normal
		&& TexCoord == other.TexCoord;
}