#include "Physics/pch.h"
#include "Physics/ParticleContacts.hpp"
#include "Physics/Particle.hpp"

//===========================================================================================================
// Particle Contact
//===========================================================================================================
void ParticleContact::Resolve(float duration) {
	ResolveVelocity(duration);
	ResolveInterpenetration(duration);
}

float ParticleContact::CalculateSeparatingVelocity() const {
	auto relativeVelocity = mpParticles[0]->GetVelocity();
	if (mpParticles[1]) relativeVelocity -= mpParticles[1]->GetVelocity();
	return relativeVelocity.Dot(mContactNormal);
}

void ParticleContact::ResolveVelocity(float duration) {
	// 접촉 방향의 속도를 구합니다.
	auto separatingVelocity = CalculateSeparatingVelocity();

	// 접촉 해소가 필요한지 검사합니다.
	if (separatingVelocity > 0.f) return; // 접촉이 분리 중이거나 정지 상태이므로 충격량이 필요하지 않습니다.

	// 새 분리 속도를 계산합니다.
	auto newSepVelocity = -separatingVelocity * mRestitution;

	// 가속도만으로 누적된 속도를 검사합니다.
	auto accCausedVelocity = mpParticles[0]->GetAcceleration();
	if (mpParticles[1]) accCausedVelocity -= mpParticles[1]->GetAcceleration();
	auto accCausedSepVelocity = accCausedVelocity.Dot(mContactNormal) * duration;

	// 누적 가속도에 의한 접근 속도가 있다면 새 분리 속도에서 제거합니다.
	if (accCausedSepVelocity < 0.f) {
		newSepVelocity += mRestitution * accCausedSepVelocity;
		// 제거할 수 있는 양보다 많이 제거하지 않았는지 확인합니다.
		if (newSepVelocity < 0.f) newSepVelocity = 0.f;
	}

	auto deltaVelocity = newSepVelocity - separatingVelocity;

	// 각 객체에 역질량에 비례하여 속도 변화량을 적용합니다. 
	// 역질량이 작을수록, 즉 실제 질량이 클수록 속도 변화량이 작습니다.
	auto totalInverseMass = mpParticles[0]->GetInverseMass();
	if (mpParticles[1]) totalInverseMass += mpParticles[1]->GetInverseMass();

	// 모든 입자의 질량이 무한대이면 충격량은 효과가 없습니다.
	if (totalInverseMass <= 0.f) return;

	// 적용할 충격량을 계산합니다.
	auto impulse = deltaVelocity / totalInverseMass;
	// 역질량 단위당 충격량을 구합니다.
	auto impulsePerIMass = mContactNormal * impulse;

	// 접촉 방향으로 역질량에 비례하는 충격량을 적용합니다.
	mpParticles[0]->SetVelocity(mpParticles[0]->GetVelocity() + impulsePerIMass * mpParticles[0]->GetInverseMass());
	// 입자 1은 반대 방향으로 움직입니다.
	if (mpParticles[1]) mpParticles[1]->SetVelocity(mpParticles[1]->GetVelocity() - impulsePerIMass * mpParticles[1]->GetInverseMass());
}

void ParticleContact::ResolveInterpenetration(float duration) {
	// 상호 침투가 없으면 해소할 필요가 없습니다.
	if (mPenetration <= 0.f) return;

	// 각 객체에 역질량에 비례하여 위치 변화량을 적용합니다. 
	// 역질량이 작을수록, 즉 실제 질량이 클수록 위치 변화량이 작습니다.
	auto totalInverseMass = mpParticles[0]->GetInverseMass();
	if (mpParticles[1]) totalInverseMass += mpParticles[1]->GetInverseMass();

	// 모든 입자의 질량이 무한대이면 위치를 변경할 수 없습니다.
	if (totalInverseMass <= 0.f) return;

	// 접촉 방향으로 이동할 거리
	auto movePerIMass = mContactNormal * (mPenetration / totalInverseMass);
	// 각 입자의 위치를 이동합니다.
	mpParticles[0]->SetPosition(mpParticles[0]->GetPosition() + movePerIMass * mpParticles[0]->GetInverseMass());
	if (mpParticles[1]) mpParticles[1]->SetPosition(mpParticles[1]->GetPosition() - movePerIMass * mpParticles[1]->GetInverseMass());
}
//-----------------------------------------------------------------------------------------------------------




//===========================================================================================================
// Particle Contact Resolver
//===========================================================================================================
ParticleContactResolver::ParticleContactResolver(unsigned iterations) 
	: mIterations(iterations), mIterationsUsed(0) {}

void ParticleContactResolver::ResolveContacts(ParticleContact** const contactArray, unsigned numContacts, float duration) {
	unsigned i;

	mIterationsUsed = 0;
	while (mIterationsUsed < mIterations) {
		// 가장 큰 접촉 속도를 가진 접촉을 찾습니다.
		auto max = std::numeric_limits<float>::max();
		auto maxIndex = numContacts;

		for (i = 0; i < numContacts; ++i) {
			auto sepVel = contactArray[i]->CalculateSeparatingVelocity();
			if (sepVel < max && (sepVel < 0.f || contactArray[i]->mPenetration > 0.f)) {
				max = sepVel;
				maxIndex = i;
			}
		}

		// 모든 접촉이 해소되었으면 종료합니다.
		if (maxIndex == numContacts) break;

		// 선택한 접촉을 해소합니다.
		contactArray[maxIndex]->Resolve(duration);

		// 다른 접촉에 영향을 미쳤을 수 있으므로 반복합니다.
		mIterationsUsed++;
	}
}
//-----------------------------------------------------------------------------------------------------------