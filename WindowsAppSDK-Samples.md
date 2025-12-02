### 🧩 앱 수명주기·백그라운드·배포

* **AppLifecycle**
  → 앱 실행·종료, 여러 인스턴스, 재시작, 공유 대상(Share Target) 같은 **앱 수명주기 기능 데모**. ([GitHub][1])

* **BackgroundTask**
  → 앱이 꺼져 있어도 동작하는 **백그라운드 작업(Background Task)** 사용 예제. ([GitHub][1])

* **DeploymentManager**
  → PC에 설치된 **Windows App SDK 런타임 상태를 확인·초기화**하는 방법 샘플. ([GitHub][1])

* **Installer**
  → 콘솔 창 없이 **Windows App SDK 설치 프로그램을 실행**하는 간단 예제. ([GitHub][1])

* **SelfContainedDeployment**
  → 앱 안에 Windows App SDK를 같이 넣어 두는 **“self-contained” 배포 방식** 샘플. ([GitHub][1])

* **Unpackaged**
  → MSIX로 패키징하지 않은 **기존 Win32 앱에서 Windows App SDK를 쓰는 방법** 데모. ([GitHub][1])

* **localpackages**
  → PC에 로컬로 있는 **패키지(앱/프레임워크)를 찾고 사용하는 샘플**(로컬 패키지 다루기 예제).

---

### 🎨 UI, 그래픽, 입력

* **Composition**
  → Microsoft.UI.Composition으로 **시각 효과·애니메이션·레이어드 UI** 만드는 예제 모음. ([GitHub][1])

* **Input**
  → 마우스·펜·터치 등 **고급 입력 처리(Microsoft.UI.Input)** 샘플. ([GitHub][1])

* **SceneGraph**
  → 새로운 **SceneGraph API로 2D/3D 장면 그래프를 구성·렌더링**하는 예제. ([GitHub][1])

* **CustomControls**
  → 재사용 가능한 **사용자 정의 WinUI 컨트롤(런타임 컴포넌트)** 만드는 방법. ([GitHub][1])

* **Mica**
  → Windows 11 스타일의 **Mica 배경 재질 효과**를 앱에서 적용하는 예제. ([GitHub][1])

* **TextRendering**
  → DWriteCore를 이용해 **텍스트를 다양한 방식으로 그리는 갤러리** 샘플. ([GitHub][1])

* **Windowing**
  → 여러 창, 크기·위치 조정 등 **Windowing API로 앱 윈도우 관리**하는 예제. ([GitHub][1])

* **Islands**
  → 기존 Win32 앱 안에 **WinUI XAML “섬(Island)”를 삽입**하는 방법. ([GitHub][1])

* **Widgets**
  → 작업 표시줄 위젯 보드에 뜨는 **Windows 위젯을 만드는 샘플**. ([GitHub][1])

* **SecureUI**
  → 스크린샷·화면 캡처를 막는 등 **민감한 내용을 보호하는 안전한 UI** 구현 예제.

* **PhotoEditor**
  → WinUI 3로 만든 **간단한 사진 편집/보기 앱 전체 예제**(실전 앱 구조 참고용). ([GitHub][1])

---

### 🔔 알림·리소스·인사이트

* **Notifications**
  → **푸시 알림**과 **앱 내부 알림(App notifications)** 을 보내고 처리하는 샘플. ([GitHub][1])

* **ResourceManagement**
  → MRT Core로 **다국어·다해상도 리소스(이미지/문자열)를 관리**하는 방법. ([GitHub][1])

* **Insights**
  → 앱에서 **사용 통계·로그·진단 정보(telemetry/insights)를 수집·분석**하는 기능 예제.

---

### 📦 설치 방식·패키지

* **Installer** / **DeploymentManager** / **SelfContainedDeployment** / **Unpackaged** / **localpackages**
  → 전부 합쳐 보면, **Windows App SDK를 어떻게 설치·찾고·같이 배포하고·기존 앱에 붙이는지**를 보여주는 배포/설치 예제 묶음이라고 보면 됩니다. ([GitHub][1])

---

### 🤖 인공지능 관련

* **WindowsAIFoundry**
  → “AI Dev Gallery” 샘플로, **Copilot Runtime·Phi Silica 등 온디바이스 AI 기능을 앱에 붙이는 예제 모음**. ([GitHub][1])

* **WindowsML**
  → ONNX 모델을 CPU/GPU/NPU에서 돌리는 **Windows ML(WinML) 추론 샘플**(C++/C#/Python 버전). ([GitHub][1])

---

### 🔧 기타

* **AppLifecycle** / **BackgroundTask** / **Input** / **Windowing** 등은
  → “실제 앱에서 자주 쓰는 시스템 기능들(앱 실행 방식, 백그라운드, 입력, 창 관리)”을 각각 독립된 예제로 보여주는 폴더라고 이해하시면 됩니다. ([GitHub][1])

---

혹시 “학생들에게 보여줄 때 어떤 샘플부터 순서대로 돌려보면 좋을지”까지 정리해 달라고 하시면,
MFC → WinUI3 전환 흐름에 맞춰 **추천 학습 순서**도 간단히 짜 드릴게요.

[1]: https://github.com/microsoft/WindowsAppSDK-Samples "GitHub - microsoft/WindowsAppSDK-Samples: Feature samples for the Windows App SDK"
