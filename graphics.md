---
# WinUI 3(C++/WinRT)에서 “그래픽스”

---

# 1) XAML 도형 & 효과 (가장 쉬움)

* 대상: 단순 도형(타원, 선, 사각형), 아이콘, 경로(Geometry) 그리기.
* 장점: 선언적(XAML)이라 생산성이 높고, 스타일/애니메이션 바로 활용 가능.
* 단점: 초당 수천 개 도형, 복잡한 실시간 그리기엔 부담.

```xml
<Grid>
  <Ellipse Width="180" Height="120" Fill="CornflowerBlue" />
  <TextBlock Text="I ♥ WinUI3" HorizontalAlignment="Center" VerticalAlignment="Center"
             FontSize="28" FontWeight="Bold"/>
</Grid>
```

언제? “UI 구성요소 수준”의 간단한 그래픽이면 이게 제일 빠릅니다.

---

# 2) Composition(시각적 레이어) – 고성능 2D, 경량 애니메이션

* 대상: 부드러운 애니메이션, 파티클/레이어 합성, 블러/색상 브러시, 벡터 쉐이프.
* 장점: UI 쓰레드 부담이 낮고 매우 부드러움. XAML 위/아래 층에 시각효과를 얹기 좋아요.
* 단점: 저수준 API라 약간의 초기 학습이 필요.

## 미니 예제(타원 하나를 Composition으로 추가)

```cpp
// MainWindow.xaml.h
#include <winrt/Microsoft.UI.Composition.h>
#include <winrt/Microsoft.UI.Xaml.Hosting.h>
#include <winrt/Windows.UI.h> // Color

// MainWindow.xaml.cpp (Loaded 후 등 적당한 곳)
using namespace winrt;
using namespace Microsoft::UI::Composition;
using namespace Microsoft::UI::Xaml::Hosting;

Compositor compositor = Window::Current().Content().VisualTreeHelper().GetOpen();
auto shapeVisual = compositor.CreateShapeVisual();
shapeVisual.Size({ 220.0f, 140.0f });

auto geo = compositor.CreateEllipseGeometry();
geo.Radius({ 110.0f, 70.0f });

auto shape = compositor.CreateSpriteShape(geo);
shape.FillBrush(compositor.CreateColorBrush(Windows::UI::ColorHelper::FromArgb(255, 100, 149, 237)));
shape.StrokeBrush(compositor.CreateColorBrush(Windows::UI::Colors::White()));
shape.StrokeThickness(2.0f);

shapeVisual.Shapes().Append(shape);

// XAML의 어떤 Panel에 붙이기 (예: x:Name="RootGrid")
ElementCompositionPreview::SetElementChildVisual(RootGrid(), shapeVisual);
```

언제? “부드러운 실시간 애니메이션/효과, 복잡하지 않은 2D”에 최적.

---

# 3) Win2D – 즉시모드 2D 렌더링(Direct2D 래퍼)

* 대상: 캔버스에 직접 그리기(벡터, 비트맵, 텍스트, 효과), 그래프/차트/스케치/툴 등.
* 장점: C++/WinRT에서도 간편. `CanvasControl`의 `Draw` 이벤트에서 매 프레임 그리기.
* 단점: 3D는 불가. 매우 많은 드로우콜/복잡한 씬은 직접 최적화 필요.

## 설치

NuGet: **Microsoft.Graphics.Win2D** (Win2D)

## XAML

```xml
<Window
  ...
  xmlns:canvas="using:Microsoft.Graphics.Canvas.UI.Xaml">
  <canvas:CanvasControl x:Name="Canvas"
                        Draw="OnCanvasDraw"
                        CreateResources="OnCreateResources"/>
</Window>
```

## C++/WinRT 코드

```cpp
#include <winrt/Microsoft.Graphics.Canvas.h>
#include <winrt/Microsoft.Graphics.Canvas.UI.Xaml.h>
#include <winrt/Windows.UI.h>

using namespace Microsoft::Graphics::Canvas;
using namespace Microsoft::Graphics::Canvas::UI::Xaml;

CanvasTextFormat m_textFmt{ nullptr };

void MainWindow::OnCreateResources(CanvasControl const&, CanvasCreateResourcesEventArgs const&)
{
    m_textFmt = CanvasTextFormat();
    m_textFmt.FontSize(28.0f);
    m_textFmt.FontWeight(Windows::UI::Text::FontWeights::Bold());
}

void MainWindow::OnCanvasDraw(CanvasControl const& sender, CanvasDrawEventArgs const& args)
{
    auto ds = args.DrawingSession();
    ds.Clear(Windows::UI::Colors::Black());
    ds.FillEllipse(110.0f, 80.0f, 90.0f, 60.0f, Windows::UI::Colors::CornflowerBlue());
    ds.DrawText(L"I ♥ WinUI3", 40.0f, 65.0f, Windows::UI::Colors::White(), m_textFmt);
}
```

언제? “커스텀 2D 드로잉(툴, 차트, 이미지 편집, 주석, 글자/도형)”이 핵심이면 Win2D가 가장 편합니다.

---

# 4) DirectX 11/12(+ Direct2D/DirectWrite) – 최저레벨 & 3D

* 대상: 3D 렌더링, 대규모/고성능 그래픽, 게임/시뮬레이션, 커스텀 파이프라인.
* 장점: 성능/자유도 최고. D2D/DWrite로 고품질 2D/텍스트도 함께.
* 단점: 가장 복잡. 디바이스/스왑체인/동기화 관리 필요.

## 패널

* **SwapChainPanel** (`Microsoft.UI.Xaml.Controls.SwapChainPanel`)에 DX 스왑체인을 연결해 화면에 출력합니다.
* UI는 XAML로, 렌더는 DirectX로—하이브리드 구성이 가능.

언제? “3D 또는 대규모 커스텀 렌더링”이 필요할 때.

---

# 5) WriteableBitmap / SoftwareBitmap – CPU 기반 픽셀 접근

* 대상: 픽셀 단위 조작(간단한 이미지 처리, 스크린샷 후 그리기).
* 장점: 진입장벽 낮음, 데이터 처리 → Image.Source로 표시.
* 단점: CPU 연산이라 대형/고빈도 갱신에선 느릴 수 있음.

---

## 어떻게 선택할까? (빠른 가이드)

* **UI 도형/효과/부드러운 애니메이션** → **Composition**
* **툴/차트/편집기 등 2D 드로잉** → **Win2D (CanvasControl)**
* **3D/최고성능/엔진수준** → **DirectX(+D2D/DWrite)**
* **간단한 픽셀 조작** → **WriteableBitmap**

---

## 자주 쓰는 패턴 팁

* **입력 처리**: PointerPressed/Released/Moved로 좌표를 받아 Win2D/Composition 상태 업데이트.
* **타이밍/루프**: `CompositionTarget::Rendering` 또는 `DispatcherQueueTimer`로 일정 FPS 갱신.
* **좌표 변환**: DPI(픽셀/뷰) 주의. Win2D는 디바이스 독립 단위를 사용해 편함.
* **성능**: 빈번한 `new`/`std::vector` 재할당 줄이고, 브러시/지오메트리 캐시. 드로우콜 최소화.

---

## “바로 시작” 추천 루트

1. **Win2D 템플릿**으로 시작해서 도형·텍스트·이미지 렌더링 느낌을 잡는다.
2. 애니메이션/효과가 필요하면 **Composition**을 얹는다.
3. 성능/3D 요구가 생기면 **DirectX**로 내려간다(필요 부분만).

---
