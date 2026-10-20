#include "Physics/pch.h"
#include "Physics/ParticleForceGenerator.hpp"
#include "Physics/Particle.hpp"

// ----------------------------------------------------------------------------------------------------------------
// Particle Gravity
// ----------------------------------------------------------------------------------------------------------------
ParticleGravity::ParticleGravity(const Vec3& gravity)
	: mGravity(gravity) {}

void ParticleGravity::UpdateForce(Particle* const particle, float duration) {
	if (!particle->HasFiniteMass()) return;
	particle->AddForce(mGravity * particle->GetMass());
}

// ----------------------------------------------------------------------------------------------------------------
// Particle Drag
// ----------------------------------------------------------------------------------------------------------------
ParticleDrag::ParticleDrag(float k1, float k2)
	: mK1(k1), mK2(k2) {}

void ParticleDrag::UpdateForce(Particle* const particle, float duration) {
	Vec3 force = particle->GetVelocity();

	float dragCoeff = force.Length();
	dragCoeff = mK1 * dragCoeff + mK2 * dragCoeff * dragCoeff;

	force.Normalize();
	force *= -dragCoeff;

	particle->AddForce(force);
}

// ----------------------------------------------------------------------------------------------------------------
// Particle Anchored Spring
// ----------------------------------------------------------------------------------------------------------------
ParticleAnchoredSpring::ParticleAnchoredSpring(const Vec3& anchor, float springConstant, float restLength)
	: mAnchor(anchor), mSpringConstant(springConstant), mRestLength(restLength) {}

void ParticleAnchoredSpring::UpdateForce(Particle* const particle, float duration) {
	Vec3 force = particle->GetPosition() - mAnchor;

	float magnitude = force.Length();
	magnitude = std::abs(mRestLength - magnitude) * mSpringConstant;

	force.Normalize();
	force *= -magnitude;

	particle->AddForce(force);
}

// ----------------------------------------------------------------------------------------------------------------
// Particle Anchored Bungee
// ----------------------------------------------------------------------------------------------------------------
ParticleAnchoredBungee::ParticleAnchoredBungee(const Vec3& anchor, float springConstant, float restLength)
	: ParticleAnchoredSpring(anchor, springConstant, restLength) {}

void ParticleAnchoredBungee::UpdateForce(Particle* const particle, float duration) {
	Vec3 force = particle->GetPosition() - mAnchor;

	float magnitude = force.Length();
	if (magnitude <= mRestLength) return;

	magnitude = (magnitude - mRestLength) * mSpringConstant;

	force.Normalize();
	force *= -magnitude;

	particle->AddForce(force);
}

// ----------------------------------------------------------------------------------------------------------------
// Particle Fake Spring
// ----------------------------------------------------------------------------------------------------------------
ParticleFakeSpring::ParticleFakeSpring(const Vec3& anchor, float springConstant, float damping)
	: mAnchor(anchor), mSpringConstant(springConstant), mDamping(damping) {}

void ParticleFakeSpring::UpdateForce(Particle* const particle, float duration) {
	if (!particle->HasFiniteMass()) return;

	Vec3 position = particle->GetPosition() - mAnchor;

	float gamma = 0.5f * std::sqrt(4.f * mSpringConstant - mDamping * mDamping);
	if (gamma == 0.f) return;

	Vec3 c = position * (mDamping / (2.f * gamma)) + particle->GetVelocity() * (1.f / gamma);
	Vec3 target = position * std::cos(gamma * duration) + c * std::sin(gamma * duration);
	target *= std::exp(-0.5f * mDamping * duration);

	Vec3 accel = (target - position) * (1.f / (duration * duration)) - particle->GetVelocity() * (1.f / duration);

	particle->AddForce(accel * particle->GetMass());
}

// ----------------------------------------------------------------------------------------------------------------
// Particle Spring
// ----------------------------------------------------------------------------------------------------------------
ParticleSpring::ParticleSpring(Particle* other, float springConstant, float restLength)
	: mOther(other), mSpringConstant(springConstant), mRestLength(restLength) {}

void ParticleSpring::UpdateForce(Particle* const particle, float duration) {
	Vec3 force = particle->GetPosition() - mOther->GetPosition();

	float magnitude = force.Length();
	magnitude = std::abs(magnitude - mRestLength) * mSpringConstant;

	force.Normalize();
	force *= -magnitude;

	particle->AddForce(force);
}

// ----------------------------------------------------------------------------------------------------------------
// Particle Bungee
// ----------------------------------------------------------------------------------------------------------------
ParticleBungee::ParticleBungee(Particle* other, float springConstant, float restLength)
	: mOther(other), mSpringConstant(springConstant), mRestLength(restLength) {}

void ParticleBungee::UpdateForce(Particle* const particle, float duration) {
	Vec3 force = particle->GetPosition() - mOther->GetPosition();

	float magnitude = force.Length();
	if (magnitude <= mRestLength) return;

	magnitude = (mRestLength - magnitude) * mSpringConstant;

	force.Normalize();
	force *= -magnitude;

	particle->AddForce(force);
}

// ----------------------------------------------------------------------------------------------------------------
// Particle Buoyancy
// ----------------------------------------------------------------------------------------------------------------
ParticleBuoyancy::ParticleBuoyancy(float maxDepth, float volume, float waterHeight, float liquidDensity)
	: mMaxDepth(maxDepth), mVolume(volume), mWaterHeight(waterHeight), mLiquidDensity(liquidDensity) {}

void ParticleBuoyancy::UpdateForce(Particle* const particle, float duration) {
	float depth = particle->GetPosition().y;
	if (depth >= mWaterHeight + mMaxDepth) return;

	Vec3 force(0.f);
	if (depth <= mWaterHeight - mMaxDepth) {
		force.y = mLiquidDensity * mVolume;
		particle->AddForce(force);
		return;
	}

	force.y = mLiquidDensity * mVolume * (depth - mMaxDepth - mWaterHeight) / (2.f * mMaxDepth);

	particle->AddForce(force);
}

// ----------------------------------------------------------------------------------------------------------------
// Particle Force Registry
// ----------------------------------------------------------------------------------------------------------------
void ParticleForceRegistry::Add(Particle* const particle, ParticleForceGenerator* const fg) {
	ParticleForceRegistration registration;
	registration.Particle = particle;
	registration.FG = fg;
	mRegistry.push_back(registration);
}

void ParticleForceRegistry::Remove(Particle* const particle, ParticleForceGenerator* const fg) {
	auto it = std::remove_if(mRegistry.begin(), mRegistry.end(),
		[particle, fg](const ParticleForceRegistration& registration) {
			return registration.Particle == particle && registration.FG == fg;
		});
	mRegistry.erase(it, mRegistry.end());
}

void ParticleForceRegistry::Clear() {
	mRegistry.clear();
}

void ParticleForceRegistry::UpdateForces(float duration) {
	for (auto i = mRegistry.begin(); i != mRegistry.end(); ++i) 
		i->FG->UpdateForce(i->Particle, duration);
}