#ifndef __AMATERIAL_INL__
#define __AMATERIAL_INL__

const Ptr<ATexture>& AMaterial::GetAlbedoMap() const noexcept { return mAlbedoMap; }

void AMaterial::SetAlbedoMap(const Ptr<ATexture>& albedoMap) noexcept { 
	mAlbedoMap = albedoMap; 
	mpNeedToUpdate = true;
}

const Vec3& AMaterial::GetAlbedo() const noexcept { return mAlbedo; }

void AMaterial::SetAlbedo(const Vec3& albedo) noexcept { 
	mAlbedo = albedo; 
	mpNeedToUpdate = true;
}

const Vec3& AMaterial::GetSpecular() const noexcept { return mSpecular; }

void AMaterial::SetSpecular(const Vec3& specular) noexcept { 
	mSpecular = specular; 
	mpNeedToUpdate = true;
}

float AMaterial::GetRoughness() const noexcept { return mRoughness; }

void AMaterial::SetRoughness(float roughness) noexcept { 
	mRoughness = roughness; 
	mpNeedToUpdate = true;
}

float AMaterial::GetMetalic() const noexcept { return mMetalic; }

void AMaterial::SetMetalic(float metalic) noexcept { 
	mMetalic = metalic; 
	mpNeedToUpdate = true;
}

ERenderDomain::Type AMaterial::GetRenderDomain() const noexcept { return mDomain; }

void AMaterial::SetRenderDomain(ERenderDomain::Type domain) noexcept { 
	mDomain = domain; 
	mpNeedToUpdate = true;
}

#endif // __AMATERIAL_INL__