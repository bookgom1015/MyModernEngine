#pragma once

class Particle;

class ParticleForceGenerator {
public:
	ParticleForceGenerator() = default;
	virtual ~ParticleForceGenerator() = default;

	DISALLOW_COPY_AND_MOVE(ParticleForceGenerator)

public:
	virtual void UpdateForce(Particle* const particle, float duration) = 0;
};

class ParticleGravity : public ParticleForceGenerator {
public:
	ParticleGravity(const Vec3& gravity);
	virtual ~ParticleGravity() = default;

	DISALLOW_COPY_AND_MOVE(ParticleGravity)

public:
	virtual void UpdateForce(Particle* const particle, float duration) override;

protected:
	Vec3 mGravity;
};

class ParticleDrag : public ParticleForceGenerator {
public:
	ParticleDrag(float k1, float k2);
	virtual ~ParticleDrag() = default;

	DISALLOW_COPY_AND_MOVE(ParticleDrag)

public:
	virtual void UpdateForce(Particle* const particle, float duration) override;

protected:
	float mK1;
	float mK2;
};

class ParticleAnchoredSpring : public ParticleForceGenerator {
public:
	ParticleAnchoredSpring(const Vec3& anchor, float springConstant, float restLength);
	virtual ~ParticleAnchoredSpring() = default;

	DISALLOW_COPY_AND_MOVE(ParticleAnchoredSpring)

public:
	virtual void UpdateForce(Particle* const particle, float duration) override;

protected:
	Vec3 mAnchor;
	float mSpringConstant;
	float mRestLength;
};

class ParticleAnchoredBungee : public ParticleAnchoredSpring {
public:
	ParticleAnchoredBungee(const Vec3& anchor, float springConstant, float restLength);
	virtual ~ParticleAnchoredBungee() = default;

	DISALLOW_COPY_AND_MOVE(ParticleAnchoredBungee)

public:
	virtual void UpdateForce(Particle* const particle, float duration) override;
};

class ParticleFakeSpring : public ParticleForceGenerator {
public:
	ParticleFakeSpring(const Vec3& anchor, float springConstant, float damping);
	virtual ~ParticleFakeSpring() = default;

	DISALLOW_COPY_AND_MOVE(ParticleFakeSpring)

public:
	virtual void UpdateForce(Particle* const particle, float duration) override;

protected:
	Vec3 mAnchor;
	float mSpringConstant;
	float mDamping;
};

class ParticleSpring : public ParticleForceGenerator {
public:
	ParticleSpring(Particle* other, float springConstant, float restLength);
	virtual ~ParticleSpring() = default;

	DISALLOW_COPY_AND_MOVE(ParticleSpring)

public:
	virtual void UpdateForce(Particle* const particle, float duration) override;

protected:
	Particle* mOther;
	float mSpringConstant;
	float mRestLength;
};

class ParticleBungee : public ParticleForceGenerator {
public:
	ParticleBungee(Particle* other, float springConstant, float restLength);
	virtual ~ParticleBungee() = default;

	DISALLOW_COPY_AND_MOVE(ParticleBungee)

public:
	virtual void UpdateForce(Particle* const particle, float duration) override;

protected:
	Particle* mOther;
	float mSpringConstant;
	float mRestLength;
};

class ParticleBuoyancy : public ParticleForceGenerator {
public:
	ParticleBuoyancy(float maxDepth, float volume, float waterHeight, float liquidDensity = 1000.f);
	virtual ~ParticleBuoyancy() = default;

	DISALLOW_COPY_AND_MOVE(ParticleBuoyancy)

public:
	virtual void UpdateForce(Particle* const particle, float duration) override;

protected:
	float mMaxDepth;
	float mVolume;
	float mWaterHeight;
	float mLiquidDensity;
};

class ParticleForceRegistry {
protected:
	struct ParticleForceRegistration {
		Particle* Particle;
		ParticleForceGenerator* FG;
	};

	using Registry = std::vector<ParticleForceRegistration>;

public:
	ParticleForceRegistry() = default;
	virtual ~ParticleForceRegistry() = default;

	DISALLOW_COPY_AND_MOVE(ParticleForceRegistry)

public:
	void Add(Particle* const particle, ParticleForceGenerator* const fg);
	void Remove(Particle* const particle, ParticleForceGenerator* const fg);
	void Clear();
	void UpdateForces(float duration);

protected:
	Registry mRegistry;
};