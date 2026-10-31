/*
 * Interface file for the contact resolution system for particles.
 *
 * Part of the Cyclone physics system.
 *
 * Copyright (c) Icosagon 2003. All Rights Reserved.
 *
 * This software is distributed under licence. Use of this software
 * implies agreement with all terms and conditions of the accompanying
 * software licence.
 */

//===========================================================================================================
// 
// @file ParticleForceGenerator.hpp
//
// 힘 생성기의 인터페이스와 예제 구현을 포함합니다.
//
//===========================================================================================================
#pragma once

class Particle;




//===========================================================================================================
// 
// 힘 생성기는 하나 이상의 입자에 힘을 더하도록 요청받을 수 있습니다.
//
//===========================================================================================================
class ParticleForceGenerator {
public:
	ParticleForceGenerator() = default;
	virtual ~ParticleForceGenerator() = default;

	DISALLOW_COPY_AND_MOVE(ParticleForceGenerator)

public:
	// 인터페이스 구현에서 이 함수를 재정의하여 주어진 입자에 작용하는 힘을 계산하고 갱신합니다.
	virtual void UpdateForce(Particle* const particle, float duration) = 0;
};




//===========================================================================================================
// 
// 중력을 적용하는 힘 생성기입니다.하나의 인스턴스를 여러 입자에 사용할 수 있습니다.
//
//===========================================================================================================
class ParticleGravity : public ParticleForceGenerator {
public:
	ParticleGravity(const Vec3& gravity);
	virtual ~ParticleGravity() = default;

	DISALLOW_COPY_AND_MOVE(ParticleGravity)

public:
	// 주어진 입자에 중력을 적용합니다.
	virtual void UpdateForce(Particle* const particle, float duration) override;

protected:
	// 중력 가속도
	Vec3 mGravity;		
};




//===========================================================================================================
// 
// 항력을 적용하는 힘 생성기입니다. 하나의 인스턴스를 여러 입자에 사용할 수 있습니다.
//
//===========================================================================================================
class ParticleDrag : public ParticleForceGenerator {
public:
	ParticleDrag(float k1, float k2);
	virtual ~ParticleDrag() = default;

	DISALLOW_COPY_AND_MOVE(ParticleDrag)

public:
	// 주어진 입자에 항력을 적용합니다.
	virtual void UpdateForce(Particle* const particle, float duration) override;

protected:
	// 속도에 비례하는 항력 계수
	float mK1;

	// 속도의 제곱에 비례하는 항력 계수
	float mK2;
};




//===========================================================================================================
// 
// 한쪽 끝이 공간의 고정점에 연결된 스프링 힘을 적용하는 힘 생성기입니다.
//
//===========================================================================================================
class ParticleAnchoredSpring : public ParticleForceGenerator {
public:
	ParticleAnchoredSpring(const Vec3& anchor, float springConstant, float restLength);
	virtual ~ParticleAnchoredSpring() = default;

	DISALLOW_COPY_AND_MOVE(ParticleAnchoredSpring)

public:
	// 스프링 속성을 설정합니다.
	void Initialize(const Vec3& anchor, float springConstant, float restLength);

	// 주어진 입자에 스프링 힘을 적용합니다.
	virtual void UpdateForce(Particle* const particle, float duration) override;

protected:
	// 스프링이 연결된 고정점
	Vec3 mAnchor;

	// 스프링 상수
	float mSpringConstant;

	// 스프링의 자연 길이
	float mRestLength;
};




//===========================================================================================================
// 
// 한쪽 끝이 공간의 고정점에 연결된 번지 줄의 힘을 적용하는 힘 생성기입니다.
// 
//===========================================================================================================
class ParticleAnchoredBungee : public ParticleAnchoredSpring {
public:
	ParticleAnchoredBungee(const Vec3& anchor, float springConstant, float restLength);
	virtual ~ParticleAnchoredBungee() = default;

	DISALLOW_COPY_AND_MOVE(ParticleAnchoredBungee)

public:
	// 주어진 입자에 스프링 힘을 적용합니다.
	virtual void UpdateForce(Particle* const particle, float duration) override;
};




//===========================================================================================================
// 
// 한쪽 끝이 공간의 고정점에 연결된 뻣뻣한 스프링의 힘을 근사하는 힘 생성기입니다.
// 
//===========================================================================================================
class ParticleFakeSpring : public ParticleForceGenerator {
public:
	ParticleFakeSpring(const Vec3& anchor, float springConstant, float damping);
	virtual ~ParticleFakeSpring() = default;

	DISALLOW_COPY_AND_MOVE(ParticleFakeSpring)

public:
	virtual void UpdateForce(Particle* const particle, float duration) override;

protected:
	// 스프링의 고정된 끝 위치
	Vec3 mAnchor;

	// 스프링 상수
	float mSpringConstant;

	// 스프링 진동의 감쇠
	float mDamping;
};




//===========================================================================================================
// 
// 스프링 힘을 적용하는 힘 생성기입니다.
// 
//===========================================================================================================
class ParticleSpring : public ParticleForceGenerator {
public:
	ParticleSpring(Particle* other, float springConstant, float restLength);
	virtual ~ParticleSpring() = default;

	DISALLOW_COPY_AND_MOVE(ParticleSpring)

public:
	// 주어진 입자에 스프링 힘을 적용합니다.
	virtual void UpdateForce(Particle* const particle, float duration) override;

protected:
	// 스프링 반대쪽 끝의 입자
	Particle* mOther;

	// 스프링 상수
	float mSpringConstant;

	// 스프링의 자연 길이
	float mRestLength;
};




//===========================================================================================================
// 
// 늘어났을 때만 스프링 힘을 적용하는 힘 생성기입니다.
// 
//===========================================================================================================
class ParticleBungee : public ParticleForceGenerator {
public:
	ParticleBungee(Particle* other, float springConstant, float restLength);
	virtual ~ParticleBungee() = default;

	DISALLOW_COPY_AND_MOVE(ParticleBungee)

public:
	// 주어진 입자에 스프링 힘을 적용합니다.
	virtual void UpdateForce(Particle* const particle, float duration) override;

protected:
	// 스프링 반대쪽 끝의 입자
	Particle* mOther;

	// 스프링 상수
	float mSpringConstant;

	// 스프링의 자연 길이
	float mRestLength;
};




//===========================================================================================================
// 
// XZ 평면과 평행한 액체 표면에 대해 부력을 적용하는 힘 생성기입니다.
// 
//===========================================================================================================
class ParticleBuoyancy : public ParticleForceGenerator {
public:
	ParticleBuoyancy(float maxDepth, float volume, float waterHeight, float liquidDensity = 1000.f);
	virtual ~ParticleBuoyancy() = default;

	DISALLOW_COPY_AND_MOVE(ParticleBuoyancy)

public:
	// 주어진 입자에 부력을 적용합니다.
	virtual void UpdateForce(Particle* const particle, float duration) override;

protected:
	// 객체에 최대 부력이 발생하는 최대 잠김 깊이
	float mMaxDepth;

	// 객체의 부피
	float mVolume;

	// y=0 위의 수면 높이입니다. 수면은 XZ 평면과 평행합니다.
	float mWaterHeight;

	// 액체의 밀도입니다. 순수한 물의 밀도는 세제곱미터당 1000kg입니다.
	float mLiquidDensity;
};




//===========================================================================================================
// 
// 모든 힘 생성기와 그 힘을 받는 입자들을 저장합니다.
// 
//===========================================================================================================
class ParticleForceRegistry {
protected:
	// 하나의 힘 생성기와 그 힘을 받는 입자를 추적합니다.
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
	// 주어진 힘 생성기를 주어진 입자에 적용하도록 등록합니다.
	void Add(Particle* const particle, ParticleForceGenerator* const fg);

	// 지정된 등록 쌍을 등록부에서 제거합니다.
	// 등록되어 있지 않으면 아무 효과도 없습니다.
	void Remove(Particle* const particle, ParticleForceGenerator* const fg);

	// 등록부의 모든 등록을 지웁니다. 입자나 힘 생성기 자체는 삭제하지 않고 그 연결 기록만 제거합니다.
	void Clear();

	// 모든 힘 생성기를 호출하여 각 입자의 힘을 갱신합니다.
	void UpdateForces(float duration);

protected:
	//등록 목록을 저장합니다.
	Registry mRegistry;
};