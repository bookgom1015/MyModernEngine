#ifndef __RENDERER_INL__
#define __RENDERER_INL__

void Renderer::SetCamera(CCamera* pCamera) noexcept { mpCamera = pCamera; }

CCamera* Renderer::GetCamera() const noexcept { return mpCamera; }

void Renderer::SetEditorCamera(CCamera* pCamera) noexcept { mpEditorCamera = pCamera; }

CCamera* Renderer::GetEditorCamera() const noexcept { return mpEditorCamera; }

#endif // __RENDERER_INL__