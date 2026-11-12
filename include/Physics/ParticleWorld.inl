#ifndef __PHYSICS_PARTICLEWORLD_INL__
#define __PHYSICS_PARTICLEWORLD_INL__

ParticleWorld::Particles& ParticleWorld::GetParticles() noexcept { return mParticles; }

ParticleWorld::ContactGenerators& ParticleWorld::GetContactGenerators() noexcept { return mContactGenerators; }

ParticleForceRegistry& ParticleWorld::GetForceRegistry() noexcept { return mRegistry; }

#endif // __PHYSICS_PARTICLEWORLD_INL__