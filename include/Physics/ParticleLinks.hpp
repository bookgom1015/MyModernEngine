/*
 * Interface file for the particle links.
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
// @file ParticleLinks.hpp
//
// 입자 사이의 연결을 나타내는 클래스들을 포함합니다.
//
//===========================================================================================================
#pragma once

#include "ParticleContacts.hpp"





//===========================================================================================================
// 
// 링크는 두 입자를 연결하며 연결 제약을 위반하면 접촉을 생성합니다.
// 케이블과 막대의 기반 클래스로 사용하며, 늘어나는 길이가 제한된 스프링의 기반 클래스로도 사용할 수 있습니다.
// 
//===========================================================================================================
class ParticleLink : public ParticleContactGenerator {
public:
	ParticleLink() = default;
	virtual ~ParticleLink() = default;

	DISALLOW_COPY_AND_MOVE(ParticleLink)

public:
	//-----------------------------------------------------------------------------------------------------------
	// 
	// 이 링크의 제약 위반을 막기 위한 접촉을 생성합니다.최대 하나의 접촉만 생성하므로 포인터는 단일 원소를 가리켜도 됩니다.
	// 케이블이 지나치게 늘어나지 않았으면 0, 접촉이 필요하면 1을 반환합니다.
	// 
	// @param limit 최소 1로 가정하며 0은 유효하지 않습니다.
	// 
	// @return 실제 기록한 접촉 수를 반환합니다.
	// 
	// @note 부모 클래스에도 같은 방식의 순수 가상 함수로 선언되어 있지만 문서화를 위해 여기에 다시 선언합니다.
	//
	virtual unsigned AddContact(ParticleContact* const contact, unsigned limit) const override = 0;
	//-----------------------------------------------------------------------------------------------------------

protected:
	// 링크의 현재 길이를 반환합니다.
	float GetCurrentLength() const;

protected:
	// 이 링크로 연결된 입자 쌍을 저장합니다.
	Particle* mpParticles[2];
};




//===========================================================================================================
// 
// 케이블은 입자 쌍을 연결하며 너무 멀어지면 접촉을 생성합니다.
// 
//===========================================================================================================
class ParticleCable : public ParticleLink {
public:
	ParticleCable() = default;
	virtual ~ParticleCable() = default;

	DISALLOW_COPY_AND_MOVE(ParticleCable)

public:
	//-----------------------------------------------------------------------------------------------------------
	// 
	// 케이블이 과도하게 늘어나지 않도록 필요한 접촉을 주어진 접촉 구조에 채웁니다.
	// 
	// @param contact 포인터는 접촉 배열의 첫 번째 빈 항목을 가리켜야 합니다.
	//
	// @param limit 배열에 기록할 수 있는 최대 접촉 수입니다.
	//
	// @return 실제 기록한 접촉 수를 반환합니다.
	//
	virtual unsigned AddContact(ParticleContact* const contact, unsigned limit) const override;
	//-----------------------------------------------------------------------------------------------------------

protected:
	// 케이블의 최대 길이
	float mMaxLength;

	// 케이블의 반발 계수, 즉 튀는 정도를 저장합니다.
	float mRestitution;
};




//===========================================================================================================
// 
// 막대는 입자 쌍을 연결하며 너무 멀어지거나 가까워지면 접촉을 생성합니다.
// 
//===========================================================================================================
class ParticleRod : public ParticleLink {
public:
	ParticleRod() = default;
	virtual ~ParticleRod() = default;

	DISALLOW_COPY_AND_MOVE(ParticleRod)

public:
	//-----------------------------------------------------------------------------------------------------------
	// 
	// 막대가 늘어나거나 압축되지 않도록 필요한 접촉을 주어진 접촉 구조에 채웁니다.
	//
	// @param contact 포인터는 접촉 배열의 첫 번째 빈 항목을 가리켜야 합니다.
	//
	// @param limit 배열에 기록할 수 있는 최대 접촉 수입니다.
	//
	// @return 실제 기록한 접촉 수를 반환합니다.
	//
	virtual unsigned AddContact(ParticleContact* const contact, unsigned limit) const override;
	//-----------------------------------------------------------------------------------------------------------

protected:
	// 막대 길이
	float  mLength;
};




//===========================================================================================================
// 
// 제약은 링크와 비슷하지만 입자를 움직이지 않는 고정점에 연결합니다.
// 
//===========================================================================================================
class ParticleConstraint : public ParticleContactGenerator {
public:
	ParticleConstraint() = default;
	virtual ~ParticleConstraint() = default;

	DISALLOW_COPY_AND_MOVE(ParticleConstraint)

public:
	//-----------------------------------------------------------------------------------------------------------
	// 
	// 이 링크의 제약 위반을 막기 위한 접촉을 생성합니다.최대 하나의 접촉만 생성하므로 포인터는 단일 원소를 가리켜도 됩니다.
	// 케이블이 지나치게 늘어나지 않았으면 0, 접촉이 필요하면 1을 반환합니다.
	// 
	// @param limit 최소 1로 가정하며 0은 유효하지 않습니다.
	// 
	// @return 실제 기록한 접촉 수를 반환합니다.
	// 
	// @note 부모 클래스에도 같은 방식의 순수 가상 함수로 선언되어 있지만 문서화를 위해 여기에 다시 선언합니다.
	//
	virtual unsigned AddContact(ParticleContact* const contact, unsigned limit) const override = 0;
	//-----------------------------------------------------------------------------------------------------------

protected:
	// 링크의 현재 길이를 반환합니다.
	float GetCurrentLength() const;

protected:
	// 이 제약으로 연결된 입자들을 저장합니다.
	Particle* mpParticle;

	// 입자가 고정되는 점
	Vec3 mAnchor;
};




//===========================================================================================================
// 
// 케이블은 입자를 고정점에 연결하며 너무 멀어지면 접촉을 생성합니다.
// 
//===========================================================================================================
class ParticleRodConstraint : public ParticleConstraint {
public:
	ParticleRodConstraint() = default;
	virtual ~ParticleRodConstraint() = default;

	DISALLOW_COPY_AND_MOVE(ParticleRodConstraint)

public:
	//-----------------------------------------------------------------------------------------------------------
	// 
	// 막대가 늘어나거나 압축되지 않도록 필요한 접촉을 주어진 접촉 구조에 채웁니다.
	//
	// @param contact 포인터는 접촉 배열의 첫 번째 빈 항목을 가리켜야 합니다.
	//
	// @param limit 배열에 기록할 수 있는 최대 접촉 수입니다.
	//
	// @return 실제 기록한 접촉 수를 반환합니다.
	//
	virtual unsigned AddContact(ParticleContact* const contact, unsigned limit) const override;
	//-----------------------------------------------------------------------------------------------------------

protected:
	// 막대 길이
	float  mLength;
};