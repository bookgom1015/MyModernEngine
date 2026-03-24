#ifndef __RENDERER_INL__
#define __RENDERER_INL__

void Renderer::SetCamera(Ptr<CCamera> pCamera) noexcept { mpCamera = pCamera; }

Ptr<CCamera> Renderer::GetCamera() const noexcept { return mpCamera; }

void Renderer::SetEditorCamera(Ptr<CCamera> pCamera) noexcept { mpEditorCamera = pCamera; }

Ptr<CCamera> Renderer::GetEditorCamera() const noexcept { return mpEditorCamera; }

#endif // __RENDERER_INL__