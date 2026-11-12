#include "Physics/pch.h"
#include "Physics/ParticleWorld.hpp"
#include "Physics/Particle.hpp"

ParticleWorld::ParticleWorld(unsigned maxContacts, unsigned iterations)
	: mMaxContacts(maxContacts),
	  mbCalculateIterations(iterations == 0),
	  mResolver(iterations) {
	mContacts.resize(maxContacts);
}

unsigned ParticleWorld::GenerateContacts() {
	auto limit = mMaxContacts;
	auto nextContact = mContacts.begin();

	for (auto generator = mContactGenerators.begin(); generator != mContactGenerators.end(); ++generator) {
		const auto Used = (*generator)->AddContact(*nextContact, limit);

		limit -= Used;
		nextContact += Used;

		// 채울 수 있는 접촉 공간이 부족합니다. 따라서 일부 접촉이 누락됩니다.
		if (limit <= 0)
			break;
	}

	// 사용한 접촉 수를 반환합니다.
	return mMaxContacts - limit;
}

void ParticleWorld::Integrate(float duration) {
	for (auto particle : mParticles) {
		// 누산기에서 모든 힘을 제거합니다.
		particle->Integrate(duration);
	}
}

void ParticleWorld::RunPhysics(float duration) {
	// 먼저 힘 생성기를 적용합니다.
	mRegistry.UpdateForces(duration);

	// 이어서 객체들을 적분합니다.
	Integrate(duration);

	// 접촉을 생성합니다.
	const auto UsedContacts = GenerateContacts();

	// 접촉을 처리합니다.
	if (UsedContacts) {
		if (mbCalculateIterations) 
			mResolver.SetIterations(UsedContacts * 2);

		mResolver.ResolveContacts(mContacts.data(), UsedContacts, duration);
	}
}

void ParticleWorld::StartFrame() {
	for (auto particle : mParticles) {
		// 누산기에서 모든 힘을 제거합니다.
		particle->ClearAccumulator();
	}
}

void GroundContacts::Initialize(ParticleWorld::Particles* particles) noexcept {
	mpParticles = particles;
}

unsigned GroundContacts::AddContact(ParticleContact* const contact, unsigned limit) const {
	unsigned count = 0;

	for (auto particle : *mpParticles) {
		const auto PositionY = particle->GetPosition().y;

		// 입자가 지면 아래에 있으면 접촉을 생성합니다.
		if (PositionY < 0.f) {
			contact->mContactNormal = Vec3::Up;
			contact->mpParticles[0] = particle;
			contact->mpParticles[1] = nullptr;
			contact->mPenetration = -PositionY;
			contact->mRestitution = 0.2f;
			++count;
		}

		if (count >= limit)
			break;
	}

	return count;
}