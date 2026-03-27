#pragma once

#include "Component.hpp"
#include "LightData.h"

class CLight : public Component {
public:
    CLight();
    virtual ~CLight();

public:
    virtual bool Final() override;

public:
    CLONE(CLight);

    virtual bool SaveToLevelFile(FILE* const pFile) override;
    virtual bool LoadFromLevelFile(FILE* const pFile) override;

public:
    Mat4 GetMat(int idx = 0) const;
    void SetMat(Mat4 mat, int idx = 0);

public:
    __forceinline const LightData& GetData() const;

    __forceinline ELight::Type GetLightType() const;
    __forceinline void SetLightType(ELight::Type type);

    __forceinline Vec3 GetLightColor() const;
    __forceinline void SetLightColor(Vec3 color);

    __forceinline Vec3 GetLightDirection() const;
    __forceinline void SetLightDirection(Vec3 dir);

    __forceinline Vec3 GetAmbient() const;
    __forceinline void SetAmbient(Vec3 ambient);

    __forceinline float GetRadius() const;
    __forceinline void SetRadius(float radius);

	__forceinline float GetAttenuationRadius() const;
	__forceinline void SetAttenuationRadius(float radius);

    __forceinline float GetInnerAngle() const;
    __forceinline void SetInnerAngle(float angle);

    __forceinline float GetOuterAngle() const;
    __forceinline void SetOuterAngle(float angle);

    __forceinline float GetLength() const;
    __forceinline void SetLength(float length);

    __forceinline void SetIntensity(float intensityy);
    __forceinline float GetIntensity() const;


    __forceinline int GetBaseIndex() const;
    __forceinline void SetBaseIndex(int idx);

    __forceinline int GetIndexStride() const;
    __forceinline void SetIndexStride(int idx);

private:
    LightData mLightData;
};

#include "CLight.inl"