#ifndef __ENGINE_INL__
#define __ENGINE_INL__

HWND Engine::GetMainWndHandle() const { return mhMainWnd; }

const Uint2& Engine::GetResolution() const noexcept { return mResolution; }

#endif // __ENGINE_INL__