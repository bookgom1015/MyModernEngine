#pragma once

#include "Asset.hpp"
#include "Layer.hpp"

class ALevel : public Asset {
public:
	ALevel();
	virtual ~ALevel();

public:
	bool Begin();

	bool Update(float dt);
	bool FixedUpdate(float dt);
	bool LateUpdate(float dt);

	bool Final();

public:
	bool AddObject(int layer, Ptr<GameObject> obj);

	bool Deregister();

	void CheckCollisionLayer(UINT layer1, UINT layer2);
	void CheckCollisionLayer(const std::wstring& layerName1, const std::wstring& layerName2);

	Ptr<GameObject> FindObjectByName(const std::wstring& name);

	void Change();

public:
	ALevel* Clone();

	virtual bool Save(const std::wstring& filePath) override;
	virtual bool Load(const std::wstring& filePath) override;

public:
	__forceinline Layer* GetLayer(int layer) noexcept;
	__forceinline UINT* GetCollisionMatrix() noexcept;
	__forceinline bool IsChanged() noexcept;

private:
	std::array<Layer, MAX_LAYER> mLayers;
	std::array<UINT, MAX_LAYER> mMatrix;

	bool mbIsChanged;
};

Layer* ALevel::GetLayer(int layer) noexcept {
	assert(0 <= layer && layer < MAX_LAYER); 
	return &mLayers[layer];
}

#include "ALevel.inl"