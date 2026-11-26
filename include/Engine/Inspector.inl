#ifndef __INSPECTOR_INL__
#define __INSPECTOR_INL__

Ptr<GameObject> Inspector::GetTargetObject() const noexcept { return mTargetObject; }

Ptr<ComponentUI> Inspector::GetComponentUI(EComponent::Type type) const noexcept {
	return mComponentUIs[type];
}

#endif // __INSPECTOR_INL__