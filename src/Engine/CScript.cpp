#include "Engine/pch.h"
#include "Engine/CScript.hpp"

#include "Engine/GameObject.hpp"

CScript::CScript() : Component(EComponent::E_Script) {}

CScript::~CScript() {}

bool CScript::Final() {	return true; }

void CScript::Destroy() {
    // 소유 객체가 있으면 안전하게 제거합니다.
	if (auto owner = GetOwner()) {
		owner->Destroy();
	}
}

void CScript::AddProperty(const std::string& name, size_t offset, Property::EType type) {

}