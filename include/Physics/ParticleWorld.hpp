/*
 * Interface file for the particle / mass aggregate world structure.
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
// @file ParticleWorld.hpp
// 
// 임의 개수의 입자 질량과 그 연결을 저장하는 구조를 정의합니다.
// 
//===========================================================================================================
#pragma once

#include "Physics/ParticleForceGenerator.hpp"
#include "Physics/ParticleLinks.hpp"




	
// 
// 입자 집합을 추적하고 모두 갱신하는 수단을 제공합니다.
// 
//===========================================================================================================
class ParticleWorld {
public:
	using Particles = std::vector<Particle*>;
	using ContactGenerators = std::vector<ParticleContactGenerator*>;
	using ParticleContacts = std::vector<ParticleContact*>;

public:
	//-----------------------------------------------------------------------------------------------------------
	// 
	// 프레임당 지정된 최대 접촉 수를 처리하는 입자 시뮬레이터를 생성합니다.
	// 접촉 해소 반복 횟수를 선택적으로 지정할 수 있으며, 생략하면 접촉 수의 두 배를 사용합니다.
	//
	// @param maxContacts 허용되는 최대 접촉 수입니다. 이 수를 초과하면 접촉이 무시됩니다.
	// 
	// @param iterations 접촉 해소 반복 횟수입니다. 0이면 접촉 수의 두 배를 사용합니다.
	//
	ParticleWorld(unsigned maxContacts, unsigned iterations = 0);
	//-----------------------------------------------------------------------------------------------------------

	virtual ~ParticleWorld() = default;

	DISALLOW_COPY_AND_MOVE(ParticleWorld)

public:
	__forceinline Particles& GetParticles() noexcept;

	__forceinline ContactGenerators& GetContactGenerators() noexcept;

	__forceinline ParticleForceRegistry& GetForceRegistry() noexcept;

public:
	//-----------------------------------------------------------------------------------------------------------
	// 
	// 등록된 각 접촉 생성기를 호출하여 접촉을 보고하게 합니다. 생성된 접촉 수를 반환합니다.
	//
	// @param none
	//
	// @return 생성된 접촉 수입니다.
	//
	unsigned GenerateContacts();
	//-----------------------------------------------------------------------------------------------------------

	/**/

	//-----------------------------------------------------------------------------------------------------------
	// 
	// 주어진 시간만큼 이 월드의 모든 입자를 적분하여 진행시킵니다.
	//
	// @param duration 적분할 시간 간격입니다. 0보다 커야 합니다.
	//
	// @return 반환값이 없습니다.
	//
	void Integrate(float duration);
	//-----------------------------------------------------------------------------------------------------------

	/**/

	//-----------------------------------------------------------------------------------------------------------
	//
	// 입자 월드의 모든 물리 계산을 처리합니다.
	//
	// @param duration 이전 적분 단계의 시간 간격입니다. 적용된 힘을 보정하는 데 사용합니다.
	//
	// @return 반환값이 없습니다.
	//
	void RunPhysics(float duration);
	//-----------------------------------------------------------------------------------------------------------

	/**/

	//-----------------------------------------------------------------------------------------------------------
	// 
	// 시뮬레이션 프레임을 위해 월드를 초기화합니다. 월드에 속한 입자의 힘 누산기를 비웁니다.
	// 호출 후 이번 프레임의 힘을 입자에 더할 수 있습니다.
	//
	// @param none
	//
	// @return 반환값이 없습니다.
	//
	void StartFrame();
	//-----------------------------------------------------------------------------------------------------------

protected:
	Particles mParticles;
	ParticleForceRegistry mRegistry;
	ParticleContactResolver mResolver;
	ContactGenerators mContactGenerators;
	ParticleContacts mContacts;

	// 허용되는 최대 접촉 수, 즉 접촉 배열의 크기를 저장합니다.
	unsigned mMaxContacts;

	// 월드가 매 프레임 접촉 해소기에 전달할 반복 횟수를 계산해야 하면 true입니다.
	bool mbCalculateIterations;
};




//===========================================================================================================
// 
// 입자 포인터의 STL 벡터를 받아 지면과의 충돌을 처리하는 접촉 생성기입니다.
// 
//===========================================================================================================
class GroundContacts : public ParticleContactGenerator {
public:
	//-----------------------------------------------------------------------------------------------------------
	//
	// 입자 포인터의 STL 벡터를 받아 지면과의 충돌을 처리하는 접촉 생성기를 생성합니다.
	//
	// @param particles 입자 포인터의 STL 벡터입니다.
	//
	// @return 반환값이 없습니다.
	//
	void Initialize(ParticleWorld::Particles* particles) noexcept;
	//-----------------------------------------------------------------------------------------------------------

	/**/

	//-----------------------------------------------------------------------------------------------------------
	//
	// ParticleContactGenerator 인터페이스를 구현하여 지면과의 충돌을 처리합니다.
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
	ParticleWorld::Particles* mpParticles;
};

#include "ParticleWorld.inl"