#ifndef __PARTICLE_INL__
#define __PARTICLE_INL__

float Particle::GetMass() const noexcept { return 1.f / mInverseMass; }

float Particle::GetInverseMass() const noexcept { return mInverseMass; }

void Particle::SetInverseMass(float inverseMass) noexcept { mInverseMass = inverseMass; }

bool Particle::HasFiniteMass() const noexcept { return mInverseMass >= 0.f; }

float Particle::GetDamping() const noexcept { return mDamping; }

void Particle::SetDamping(float damping) noexcept { mDamping = damping; }

const Vec3& Particle::GetPosition() const noexcept { return mPosition; }

void Particle::SetPosition(const Vec3& position) noexcept { mPosition = position; }

const Vec3& Particle::GetVelocity() const noexcept { return mVelocity; }

void Particle::SetVelocity(const Vec3& velocity) noexcept { mVelocity = velocity; }

const Vec3& Particle::GetAcceleration() const noexcept { return mAcceleration; }

void Particle::SetAcceleration(const Vec3& acceleration) noexcept { mAcceleration = acceleration; }

void Particle::ClearAccumulator() noexcept { mForceAccumulator = Vec3(0.f); }

void Particle::AddForce(const Vec3& force) noexcept { mForceAccumulator += force; }

#endif // __PARTICLE_INL__