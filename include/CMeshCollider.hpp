#pragma once

#include "CCollider.hpp"

#include "AMesh.hpp"

class CMeshCollider : public CCollider {
public:
	CMeshCollider();
	virtual ~CMeshCollider();

public:
	virtual bool Final() override;

public:
	CLONE(CMeshCollider);

	virtual bool SaveToLevelFile(FILE* const pFile) override;
	virtual bool LoadFromLevelFile(FILE* const pFile) override;

private:
	EMeshCollision::Type mMeshCollisionType;
	Ptr<AMesh> mMesh;
};