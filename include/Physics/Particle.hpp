/*
 * Interface file for the particle class.
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
// @file Particle.hpp
//
// 더 간단한 시뮬레이션이나 조립체에서 강체 대신 사용할 수 있는 입자 클래스의 정의를 포함합니다.
// 
//===========================================================================================================
#pragma once




//===========================================================================================================
// 
// 입자는 물리 시스템에서 시뮬레이션할 수 있는 가장 단순한 객체입니다.
//
// 방향 데이터 없이 위치와 속도를 가집니다.시간에 따라 적분할 수 있으며 선형 힘과 충격량을 적용할 수 있습니다.
// 입자는 자신의 상태를 관리하고 메서드를 통해 접근을 허용합니다.
// 
//===========================================================================================================
class Particle {
public:
	Particle();
	virtual ~Particle();

	DISALLOW_COPY_AND_MOVE(Particle)

public:
	__forceinline float GetMass() const noexcept;

	__forceinline float GetInverseMass() const noexcept;
	__forceinline void SetInverseMass(float inverseMass) noexcept;

	__forceinline bool HasFiniteMass() const noexcept;

	__forceinline float GetDamping() const noexcept;
	__forceinline void SetDamping(float damping) noexcept;

	__forceinline const Vec3& GetPosition() const noexcept;
	__forceinline void SetPosition(const Vec3& position) noexcept;

	__forceinline const Vec3& GetVelocity() const noexcept;
	__forceinline void SetVelocity(const Vec3& velocity) noexcept;

	__forceinline  const Vec3& GetAcceleration() const noexcept;
	__forceinline void SetAcceleration(const Vec3& acceleration) noexcept;

	/**/

	//-----------------------------------------------------------------------------------------------------------
	// 
	// 입자에 적용할 누적 힘을 가져옵니다. 이 힘은 적분 단계에서 0으로 초기화됩니다.
	// 
	//  @param none 
	// 
	//  @return none
	//
	__forceinline void ClearAccumulator() noexcept;
	//-----------------------------------------------------------------------------------------------------------

	/**/

	//-----------------------------------------------------------------------------------------------------------
	// 
	// 다음 반복에서만 적용할 힘을 입자에 더합니다.
	// 
	// @param force 적용할 힘입니다.
	// 
	// @return none
	//
	__forceinline void AddForce(const Vec3& force) noexcept;
	//-----------------------------------------------------------------------------------------------------------

public:
	//===========================================================================================================
	// 
	// @name 입자 상태 접근 함수
	// 
	// 입자의 특성이나 상태에 접근합니다.
	// 
	//===========================================================================================================
	/**/
	//-----------------------------------------------------------------------------------------------------------
	// 
	// 입자의 질량을 설정합니다.
	// 
	// @param mass 물체의 새 질량으로, 0일 수 없습니다.작은 질량은 시뮬레이션에서 강체를 불안정하게 만들 수 있습니다.
	// 
	// @return none
	//
	// @warning 입자의 내부 데이터가 무효화됩니다.입자의 설정 값을 
	// 조회하기 전에 적분 함수 또는 calculateInternals를 호출해야 합니다.
	// 
	void SetMass(float mass);
	//-----------------------------------------------------------------------------------------------------------

	/**/

	//===========================================================================================================
	// 
	// @name 적분 및 시뮬레이션 함수
	// 
	// 시간에 따른 입자 운동을 시뮬레이션합니다. 일반적인 애플리케이션은
	// 하나 이상의 강체를 설정하고 중력 같은 지속적인 힘을 적용한 뒤
	// 매 프레임 일시적인 힘을 추가하고 렌더링 전에 적분합니다.
	// 
	// 현재 제공되는 적분 함수는 1차 뉴턴-오일러 방법뿐입니다.
	// 
	//===========================================================================================================
	/**/
	//-----------------------------------------------------------------------------------------------------------
	// 
	// 주어진 시간만큼 입자를 적분하여 진행시킵니다. 뉴턴-오일러 적분법을 사용하며
	// 정확한 적분에 대한 선형 근사이므로 경우에 따라 부정확할 수 있습니다.
	// 
	// @param duration 적분할 시간 간격입니다. 0보다 커야 합니다.
	// 
	// @return none
	//
	void Integrate(float duration);	
	//-----------------------------------------------------------------------------------------------------------

protected:
	// 입자 질량의 역수를 저장합니다. 역질량은 적분을 단순하게 합니다.
	// 또한 실시간 시뮬레이션에서는 질량이 0인 물체(수치적으로 매우 불안정)보다
	// 질량이 무한대인 물체(움직일 수 없음)가 더 유용하므로 역질량을 저장합니다.
	float mInverseMass;

	// 선형 운동에 적용할 감쇠량을 저장합니다.적분기의 수치적 불안정성으로 
	// 추가된 에너지를 제거하기 위해 감쇠가 필요합니다.
	float mDamping;

	// 월드 공간에서 입자의 위치
	Vec3 mPosition;

	// 월드 공간에서 입자의 선형 속도
	Vec3 mVelocity;

	// 다음 시뮬레이션 반복에서만 적용할 누적 힘을 저장합니다.
	// 각 적분 단계에서 0으로 초기화됩니다.
	Vec3 mForceAccumulator;

	// 입자의 가속도를 저장합니다.주로 중력 가속도를 설정하는 데 사용하며
	// 다른 일정한 가속도에도 사용할 수 있습니다.
	Vec3 mAcceleration;
};

#include "Particle.inl"