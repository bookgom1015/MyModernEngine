#ifndef __CSKYSPHERERENDER_INL__
#define __CSKYSPHERERENDER_INL__

void CSkySphereRender::SetEnvironmentCubeMap(Ptr<ATexture> environmentCubeMap) {
	mEnvironmentCubeMap = environmentCubeMap;
}
Ptr<ATexture> CSkySphereRender::GetEnvironmentCubeMap() const { return mEnvironmentCubeMap; }

#endif // __CSKYSPHERERENDER_INL__