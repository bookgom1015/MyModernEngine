# MyModernEngine

C++20으로 개발 중인 Windows 기반 3D 엔진 및 에디터입니다. **Direct3D 12 기반의 Deferred Rendering**을 중심으로 PBR, 환경 조명, 앰비언트 오클루전, 시간적 필터링과 다양한 후처리 효과를 구현하고 있습니다.

렌더러와 에디터를 백엔드별로 분리하고 있으며, **D3D12에 더해 D3D11과 Vulkan 백엔드를 추가 중**입니다.

## 그래픽스 백엔드

| 백엔드 | 상태 | 구현 범위 |
| --- | --- | --- |
| **Direct3D 12** | 주요 렌더링 기능 구현 | Deferred Rendering, PBR / IBL, 그림자, SSAO / SVGF, 볼류메트릭 조명, 후처리, 에디터 렌더링 |
| **Direct3D 11** | 추가 중 | 솔루션 빌드 구성 마련. 렌더러 및 에디터 구현 예정 |
| **Vulkan** | 추가 중 | 디바이스, 스왑 체인, 큐, 커맨드 버퍼 / 풀, 프레임 컨텍스트와 동기화 기반 구현 진행 |

아래 렌더링 기능과 패스 목록은 D3D12 구현을 기준으로 합니다. D3D11과 Vulkan의 기능 동등성은 아직 제공하지 않습니다.

## 주요 기능

- **렌더링** — G-Buffer 기반 지연 렌더링, BRDF 기반 PBR 조명, HDR, IBL 및 Reflection Probe 베이킹
- **애니메이션** — glTF / GLB 로딩, 스켈레톤과 애니메이션 클립, 본 팔레트 기반 스키닝
- **씬 구성** — GameObject / Component 구조, 부모·자식 Transform, 레이어와 레벨 저장·불러오기
- **에디터** — Dear ImGui 기반 Scene, Outliner, Inspector, Content, Profiler, Frame Viewer, Log 및 렌더링 설정 UI
- **물리** — Rigidbody와 Box / Sphere / Capsule / Mesh Collider 컴포넌트
- **에셋 관리** — 메시·머티리얼·텍스처·사운드·레벨 관리 및 에셋 디렉터리 변경 감지
- **오디오** — FMOD 기반 사운드 관리

## Renderer — 구현된 렌더 패스

렌더 패스는 `D3D12RenderPass`를 기반으로 분리되어 있으며, `D3D12RenderPassManager`에서 셰이더 컴파일, 루트 시그니처, PSO, 디스크립터 및 리사이즈 처리를 관리합니다. 개별 효과는 렌더링 설정에 따라 선택적으로 실행됩니다.

### 지오메트리 · 조명 · 환경

| 패스 | 구현 내용 | 클래스 |
| --- | --- | --- |
| G-Buffer | 정적·스키닝 메시의 Albedo, Normal, 재질 파라미터, Position, Velocity 및 재투영용 Normal / Depth 기록 | `D3D12GBuffer` |
| BRDF Lighting | G-Buffer와 조명·AO·환경 데이터를 이용한 PBR 조명 합성 | `D3D12Brdf` |
| Shadow | 정적·스키닝 메시의 깊이를 섀도 맵에 렌더링 | `D3D12Shadow` |
| Environment / IBL | Sky Sphere, BRDF LUT 생성, 환경 큐브 맵, Diffuse / Specular Irradiance 및 Reflection Probe 베이킹 | `D3D12EnvironmentManager` |
| SSAO | 화면 공간의 Normal / Depth와 Position을 이용한 앰비언트 오클루전 | `D3D12Ssao` |
| SVGF Denoising | 이전 프레임 재투영, 시간적 누적, 평균·분산 계산 및 À-Trous 필터링 | `D3D12Svgf` |
| Volumetric Light | 볼류메트릭 안개 생성 및 장면 합성 | `D3D12VolumetricLight` |

### 후처리

| 패스 | 구현 내용 | 클래스 |
| --- | --- | --- |
| Bloom | 밝은 영역을 추출·필터링하여 HDR 장면에 빛 번짐 합성 | `D3D12Bloom` |
| Eye Adaptation | 장면 휘도에 따른 노출 적응 | `D3D12EyeAdaption` |
| Tone Mapping | HDR 장면을 화면 출력 범위로 변환 | `D3D12ToneMapping` |
| Gamma Correction | 감마 보정 | `D3D12GammaCorrection` |
| Temporal Anti-Aliasing | 이전 프레임과 Velocity를 활용한 시간적 안티앨리어싱 | `D3D12Taa` |
| Motion Blur | Depth와 Velocity를 활용한 모션 블러 | `D3D12MotionBlur` |
| Vignette | 화면 주변부 비네트 효과 | `D3D12Vignette` |

### 보조 · 에디터 패스

| 패스 | 구현 내용 | 클래스 |
| --- | --- | --- |
| Texture Scaling | 텍스처 다운샘플링 | `D3D12TextureScaler` |
| Blur Filter | 후처리에서 사용하는 블러 필터 | `D3D12BlurFilter` |
| Gizmo | 에디터 조작용 축과 캡 렌더링 | `D3D12Gizmo` |
| Debug | 디버그 라인 렌더링 | `D3D12Debug` |

**개발 중: RTAO (Ray-Traced Ambient Occlusion)** — `D3D12Rtao`에 DXR 파이프라인, 셰이더 테이블 및 리소스 구성이 마련되어 있습니다. 현재 AO 실행 함수와 렌더러의 Raytracing AO 분기는 비어 있어, 완료된 렌더 패스 목록에서는 제외했습니다.

### 프레임 처리 흐름

```text
G-Buffer → Shadow → SSAO + SVGF → BRDF Lighting → Sky Sphere
    → Volumetric Light → Bloom → Eye Adaptation → Tone Mapping
    → Gamma Correction → TAA → Motion Blur → Vignette
    → Gizmo / Debug → Editor UI → Present
```

선택형 효과를 켰을 때의 대표 흐름입니다. BRDF LUT 생성과 Reflection Probe 베이킹 등 환경 데이터 준비 작업은 별도로 수행합니다.

## 빌드 및 실행

### 개발 환경

- Windows / x64
- C++20 및 **MSVC v145** 도구 집합을 지원하는 Visual Studio
- Windows SDK (`WindowsTargetPlatformVersion`: `10.0`)
- Direct3D Feature Level **12_1** 이상을 지원하는 GPU 및 드라이버
- HLSL 컴파일을 위한 DirectX Shader Compiler (DXC)

### 외부 의존성

현재 프로젝트는 저장소 루트의 `externs/`에 외부 라이브러리가 배치된 구성을 사용합니다. **`externs/`와 빌드된 `lib/`는 Git 추적 대상에서 제외되어 있으므로, 저장소를 복제한 뒤 별도로 준비해야 합니다.**

| 의존성 | 용도 / 프로젝트에서 참조하는 경로 |
| --- | --- |
| Dear ImGui | 에디터 UI — `externs/imgui/` |
| DirectXTK12 | DirectX 보조 기능 — `externs/DirectXTK12/` |
| DirectXTex | 텍스처 처리 — `externs/DirectXTex/` |
| tinygltf | glTF / GLB 로딩 — `externs/tinygltf/` |
| FMOD Core API | 오디오 — `externs/FMOD/api/core/` |
| Microsoft.Direct3D.D3D12 | D3D12 헤더 및 관련 파일 — `externs/Microsoft.Direct3D.D3D12/` |
| Vulkan SDK | Vulkan 백엔드 개발 — `externs/VulkanSDK/1.3.250.0/` |

DirectXTK12와 DirectXTex의 라이브러리는 프로젝트의 추가 라이브러리 디렉터리에 맞춰 Debug / Release, x64 구성으로 준비해야 합니다. DXC 라이브러리와 실행에 필요한 DLL도 프로젝트 설정 및 실행 환경에 맞게 준비합니다.

### 실행 순서

1. 저장소를 복제하고 위 의존성을 `externs/`의 참조 경로에 배치합니다.
2. Visual Studio에서 `MyModernEngine.slnx`를 엽니다.
3. 솔루션 구성을 **`D3D12_Debug` 또는 `D3D12_Release`**, 플랫폼을 **`x64`**로 선택합니다.
4. `Engine`을 시작 프로젝트로 설정하고 솔루션을 빌드합니다.
5. 디버깅 작업 디렉터리를 `$(SolutionDir)bin\$(Configuration)\`로 설정한 뒤 실행합니다.

실행 파일은 `bin/<Configuration>/`에 생성됩니다. 에셋 경로는 작업 디렉터리에서 두 단계 위의 `assets/`를 기준으로 계산하므로, 실행 시 이 디렉터리 구조를 유지해야 합니다. FMOD DLL은 빌드 후 이벤트에서 출력 폴더로 복사합니다.

> D3D11 / Vulkan 구성은 개발 중입니다. 현재 렌더링 기능을 살펴보려면 D3D12 구성을 사용하세요. 위 안내는 프로젝트 설정을 기준으로 하며, 별도 의존성 설치 자동화는 제공하지 않습니다.

## 디렉터리 구조

```text
MyModernEngine/
├─ MyModernEngine.slnx       # Visual Studio 솔루션
├─ build/                   # Engine 및 백엔드별 Renderer / Editor 프로젝트
├─ include/                 # 엔진·에디터 헤더 및 렌더러 인터페이스
│  └─ Renderer/
│     ├─ D3D12/
│     └─ VK/
├─ src/                     # 엔진·에디터 구현
│  └─ Renderer/
│     ├─ D3D12/             # 렌더 패스와 D3D12 리소스 관리
│     └─ VK/                # 개발 중인 Vulkan 백엔드
├─ assets/                  # glTF, 레벨, 셰이더, 사운드, 텍스처
├─ externs/                 # 외부 의존성 (별도 준비)
├─ lib/                     # 라이브러리 출력
└─ bin/                     # 실행 파일 출력
```

## 개발 중인 항목

- D3D11 백엔드 추가
- Vulkan 백엔드의 프레임 실행 및 렌더링 기능 확장
- RTAO 실행 경로 및 렌더러 통합
