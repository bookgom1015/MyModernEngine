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

#pragma once

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

	__forceinline  void ClearAccumulator() noexcept;

	__forceinline void AddForce(const Vec3& force) noexcept;

public:
	void SetMass(float mass);

	void Integrate(float duration);

private:
	float mInverseMass;
	float mDamping;
	Vec3 mPosition;
	Vec3 mVelocity;
	Vec3 mForceAccumulator;
	Vec3 mAcceleration;
};

#include "Particle.inl"