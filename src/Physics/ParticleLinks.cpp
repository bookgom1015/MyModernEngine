#include "Physics/pch.h"
#include "Physics/ParticleLinks.hpp"
#include "Physics/Particle.hpp"

float ParticleLink::GetCurrentLength() const {
	// 연결된 입자가 유효한지 확인합니다.
	if (!mpParticles[0] || !mpParticles[1]) return 0.0f; // 연결된 입자가 없으면 길이를 0으로 반환합니다.

	// 두 입자 사이의 거리를 계산합니다.
	// 여기서는 단순히 위치 벡터의 차이의 길이를 반환한다고 가정합니다.
	return (mpParticles[0]->GetPosition() - mpParticles[1]->GetPosition()).Length();
}

unsigned ParticleCable::AddContact(ParticleContact* const contact, unsigned limit) const {
	// 연결된 입자가 유효한지 확인합니다.
	if (!mpParticles[0] || !mpParticles[1]) return 0; // 연결된 입자가 없으면 접촉을 생성하지 않습니다.

	// 현재 길이를 계산합니다.
	float currentLength = GetCurrentLength();
	// 케이블이 최대 길이를 초과했는지 확인합니다.
	if (currentLength < mMaxLength) return 0; // 케이블이 늘어나지 않았으므로 접촉을 생성하지 않습니다.

	// 접촉을 생성합니다.
	contact->mpParticles[0] = mpParticles[0];
	contact->mpParticles[1] = mpParticles[1];

	// 접촉의 법선 방향을 계산합니다. 입자 0에서 입자 1로 향하는 방향입니다.
	Vec3 normal = mpParticles[1]->GetPosition() - mpParticles[0]->GetPosition();
	normal.Normalize();

	contact->mContactNormal = normal;
	// 침투 깊이는 현재 길이와 최대 길이의 차이입니다.
	contact->mPenetration = currentLength - mMaxLength;
	// 반발 계수를 설정합니다.
	contact->mRestitution = mRestitution;

	return 1; // 하나의 접촉을 생성했습니다.
}

float ParticleConstraint::GetCurrentLength() const {
	// 연결된 입자가 유효한지 확인합니다.
	if (!mpParticle) return 0.0f; // 연결된 입자가 없으면 길이를 0으로 반환합니다.

	// 입자와 고정점 사이의 거리를 계산합니다.
	return (mpParticle->GetPosition() - mAnchor).Length();
}