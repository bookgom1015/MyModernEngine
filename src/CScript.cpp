#include "pch.h"
#include "CScript.hpp"

#include "GameObject.hpp"

CScript::CScript() : Component(EComponent::E_Script) {}

CScript::~CScript() {}

bool CScript::Final() {	return true; }

void CScript::Destroy() {
    // Safely destroy the owner if present
	if (auto owner = GetOwner()) {
		owner->Destroy();
	}
}

void CScript::AddProperty(const std::string& name, size_t offset, Property::EType type) {

}