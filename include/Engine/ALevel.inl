#ifndef __ALEVEL_INL__
#define __ALEVEL_INL__

UINT* ALevel::GetCollisionMatrix() noexcept { return mMatrix.data(); }

bool ALevel::IsChanged() noexcept {
	bool changed = mbIsChanged;
	mbIsChanged = false;

	return changed;
}

#endif // __ALEVEL_INL__