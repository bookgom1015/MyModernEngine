#include "pch.h"
#include "RigidbodyUI.hpp"

RigidbodyUI::RigidbodyUI() 
	: ComponentUI{ EComponent::E_Rigidbody, "RigidbodyUI"} {}

RigidbodyUI::~RigidbodyUI() {}

void RigidbodyUI::DrawUI() {
	OutputTitle("Rigidbody");
}