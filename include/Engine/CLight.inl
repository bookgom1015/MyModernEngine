#ifndef __CLIGHT_INL__
#define	__CLIGHT_INL__

const LightData& CLight::GetData() const { return mLightData; }

ELight::Type CLight::GetLightType() const { return static_cast<ELight::Type>(mLightData.Type); }
void CLight::SetLightType(ELight::Type type) { mLightData.Type = static_cast<UINT>(type); }

Vec3 CLight::GetLightColor() const { return mLightData.Color; }
void CLight::SetLightColor(Vec3 color) { mLightData.Color = color; }

Vec3 CLight::GetLightDirection() const { return mLightData.Direction; }
void CLight::SetLightDirection(Vec3 dir) { mLightData.Direction = dir; }

Vec3 CLight::GetAmbient() const { return mLightData.AmbientColor; }
void CLight::SetAmbient(Vec3 ambient) { mLightData.AmbientColor = ambient; }

float CLight::GetRadius() const { return mLightData.Radius; }
void CLight::SetRadius(float radius) { mLightData.Radius = radius; }

float CLight::GetAttenuationRadius() const { return mLightData.AttenuationRadius; }
void CLight::SetAttenuationRadius(float radius) { mLightData.AttenuationRadius = radius; }

float CLight::GetInnerAngle() const { return mLightData.InnerConeAngle; }
void CLight::SetInnerAngle(float angle) { mLightData.InnerConeAngle = angle; }

float CLight::GetOuterAngle() const { return mLightData.OuterConeAngle; }
void CLight::SetOuterAngle(float angle) { mLightData.OuterConeAngle = angle; }

float CLight::GetLength() const { return 0.f; }
void CLight::SetLength(float length) {}

void CLight::SetIntensity(float intensity) { mLightData.Intensity = intensity; }
float CLight::GetIntensity() const { return mLightData.Intensity; }

int CLight::GetBaseIndex() const { return mLightData.BaseIndex; }
void CLight::SetBaseIndex(int idx) { mLightData.BaseIndex = idx; }

int CLight::GetIndexStride() const { return mLightData.IndexStride; }
void CLight::SetIndexStride(int idx) { mLightData.IndexStride = idx; }

#endif // __CLIGHT_INL__