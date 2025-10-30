좋아요! WinUI 3(C++/WinRT)에서 마우스 위치에 작은 원(도트)을 “그리듯이” 찍는 가장 단순한 예제를 드릴게요.
왼쪽 버튼을 누른 상태에서 마우스를 움직이면 해당 위치에 원이 계속 찍힙니다. (버튼을 떼면 중지)

아래 3파일만 교체/추가하시면 됩니다.

---

## MainWindow.xaml

```xml
<Window
    x:Class="App1.MainWindow"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:d="http://schemas.microsoft.com/expression/blend/2008"
    xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006"
    mc:Ignorable="d"
    Title="Draw Circles with Mouse"
    Width="900" Height="600">

    <Grid>
        <!-- 마우스 이벤트를 받기 위해 배경을 설정 (투명 X) -->
        <Canvas x:Name="DrawArea"
                Background="{ThemeResource SolidBackgroundFillColorBaseBrush}"
                PointerPressed="Canvas_PointerPressed"
                PointerMoved="Canvas_PointerMoved"
                PointerReleased="Canvas_PointerReleased"
                PointerExited="Canvas_PointerExited"/>
    </Grid>
</Window>
```

---

## MainWindow.xaml.h

```cpp
#pragma once
#include "MainWindow.g.h"

namespace winrt::App1::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        // 포인터(마우스) 핸들러
        void Canvas_PointerPressed(winrt::Windows::Foundation::IInspectable const& sender,
                                   winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
        void Canvas_PointerMoved(winrt::Windows::Foundation::IInspectable const& sender,
                                 winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
        void Canvas_PointerReleased(winrt::Windows::Foundation::IInspectable const& sender,
                                    winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
        void Canvas_PointerExited(winrt::Windows::Foundation::IInspectable const& sender,
                                  winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);

    private:
        void DrawDotAt(winrt::Windows::Foundation::Point const& pt);

        bool m_isDrawing{ false };
        double m_radius{ 8.0 }; // 점(원) 반지름
    };
}

namespace winrt::App1::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
```

---

## MainWindow.xaml.cpp

```cpp
#include "pch.h"
#include "MainWindow.xaml.h"

#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Input.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Shapes.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Shapes;
using winrt::Windows::Foundation::Point;

namespace winrt::App1::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();
    }

    void MainWindow::Canvas_PointerPressed(IInspectable const& /*sender*/,
                                           PointerRoutedEventArgs const& e)
    {
        m_isDrawing = true;

        // 캡처: 드래그 중에도 연속 입력
        DrawArea().CapturePointer(e.Pointer());

        // 누른 지점에도 바로 하나 찍기
        auto pt = e.GetCurrentPoint(DrawArea()).Position();
        DrawDotAt(pt);
    }

    void MainWindow::Canvas_PointerMoved(IInspectable const& /*sender*/,
                                         PointerRoutedEventArgs const& e)
    {
        if (!m_isDrawing) return;

        // 왼쪽 버튼 유지 여부(터치/펜 고려 없이 간단하게 플래그만 사용)
        auto pt = e.GetCurrentPoint(DrawArea()).Position();
        DrawDotAt(pt);
    }

    void MainWindow::Canvas_PointerReleased(IInspectable const& /*sender*/,
                                            PointerRoutedEventArgs const& e)
    {
        m_isDrawing = false;
        DrawArea().ReleasePointerCapture(e.Pointer());
    }

    void MainWindow::Canvas_PointerExited(IInspectable const& /*sender*/,
                                          PointerRoutedEventArgs const& /*e*/)
    {
        // 캔버스 밖으로 나가면 중지
        m_isDrawing = false;
    }

    void MainWindow::DrawDotAt(Point const& pt)
    {
        // 원(도트) 생성
        Ellipse dot{};
        double diameter = m_radius * 2.0;
        dot.Width(diameter);
        dot.Height(diameter);

        // 테마 친화적인 색상(라인: 시스템 강조색, 내부: 반투명 강조색)
        dot.Stroke(SolidColorBrush{ Application::Current().Resources().Lookup(box_value(L"SystemAccentColor")).
            as<winrt::Windows::UI::Color>() });
        dot.StrokeThickness(1.0);

        // 채우기 색상은 강조색에서 투명도만 낮춘 브러시
        auto accent = Application::Current().Resources().Lookup(box_value(L"SystemAccentColor")).as<winrt::Windows::UI::Color>();
        dot.Fill(SolidColorBrush{ Windows::UI::Color{ 128, accent.R, accent.G, accent.B } });

        // 좌표(왼쪽/위)는 중심이 아니라 좌상단이므로 반지름만큼 보정
        Canvas::SetLeft(dot, pt.X - m_radius);
        Canvas::SetTop(dot,  pt.Y - m_radius);

        // 캔버스에 추가
        DrawArea().Children().Append(dot);
    }
}
```

---

### 메모

* **Win2D 불필요**: 기본 `Canvas` + `Ellipse`만으로 충분합니다.
* 원 크기를 바꾸고 싶으면 `m_radius`만 조정하세요.
* “그리기”가 아니라 “하나만 이동하는 원”을 원하시면, `Ellipse`를 하나만 생성해 위치만 갱신하는 방식으로 바꿔드릴 수 있어요.
