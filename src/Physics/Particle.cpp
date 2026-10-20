#include "Physics/pch.h"
#include "Physics/Particle.hpp"

void Particle::SetMass(float mass) {
	assert(mass != 0.0f && "Mass must be non-zero");
	mInverseMass = 1.f / mass;
}

void Particle::Integrate(float duration) {
	if (mInverseMass <= 0.0f) return;

	assert(duration > 0.0f && "Duration must be positive");

	mPosition += mVelocity * duration;

	Vec3 resultingAcc = mAcceleration;
	resultingAcc += mForceAccumulator * mInverseMass;

	mVelocity += resultingAcc * duration;
	mVelocity *= std::pow(mDamping, duration);

	ClearAccumulator();
}