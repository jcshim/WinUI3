```
<?xml version="1.0" encoding="utf-8"?>
<Window
    x:Class="App1.MainWindow"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:local="using:App1"
    xmlns:d="http://schemas.microsoft.com/expression/blend/2008"
    xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006"
    mc:Ignorable="d"
    Title="App1">

    <!-- 2행 레이아웃: [0] 툴바  [1] 그리기 영역 -->
    <Grid Width="640" Height="480"
          Background="{ThemeResource ApplicationPageBackgroundThemeBrush}">
        <Grid.RowDefinitions>
            <RowDefinition Height="Auto"/>
            <RowDefinition Height="*"/>
        </Grid.RowDefinitions>

        <!-- 상단 도구막대 -->
        <StackPanel Grid.Row="0" Orientation="Horizontal" Spacing="12" Padding="8">

            <!-- 색상 버튼 + 플라이아웃(ColorPicker) -->
            <Button x:Name="ColorButton" Content="색상" VerticalAlignment="Center">
                <Button.Flyout>
                    <Flyout x:Name="ColorFlyout" Placement="Bottom">
                        <StackPanel Orientation="Vertical" Spacing="8" Padding="8">
                            <TextBlock Text="펜 색상 선택" Margin="0,0,0,4"/>
                            <ColorPicker x:Name="PenColorPicker"
                                         IsAlphaEnabled="False"
                                         IsMoreButtonVisible="False"
                                         MinWidth="220"
                                         Color="#FF000000"
                                         ColorChanged="PenColorPicker_ColorChanged"/>
                        </StackPanel>
                    </Flyout>
                </Button.Flyout>
            </Button>

            <!-- 굵기 슬라이더 -->
            <TextBlock Text="굵기" VerticalAlignment="Center" Margin="16,0,4,0"/>
            <Slider x:Name="ThicknessSlider"
                    Minimum="0.5" Maximum="24" StepFrequency="0.5"
                    Value="2"
                    Width="160"
                    ValueChanged="ThicknessSlider_ValueChanged"/>
            <TextBlock x:Name="ThicknessValueText"
                       VerticalAlignment="Center"
                       Text="2.0 px" Margin="6,0,0,0"/>
        </StackPanel>

        <!-- 그리기 표면 -->
        <Canvas Grid.Row="1"
                x:Name="DrawCanvas"
                Background="Transparent"
                PointerPressed="DrawCanvas_PointerPressed"
                PointerMoved="DrawCanvas_PointerMoved"
                PointerReleased="DrawCanvas_PointerReleased"
                PointerCanceled="DrawCanvas_PointerReleased"
                PointerCaptureLost="DrawCanvas_PointerReleased"/>
    </Grid>
</Window>
```

```
#pragma once
#include "MainWindow.g.h"

#include <winrt/Windows.UI.h>                              // Windows::UI::Color
#include <winrt/Microsoft.UI.Xaml.Shapes.h>
#include <winrt/Microsoft.UI.Xaml.Input.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>   // Slider ValueChanged args

namespace winrt::App1::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        // 그리기(포인터) 이벤트
        void DrawCanvas_PointerPressed(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
        void DrawCanvas_PointerMoved(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
        void DrawCanvas_PointerReleased(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);

        // 툴바 이벤트
        void PenColorPicker_ColorChanged(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::Controls::ColorChangedEventArgs const& e);
        void ThicknessSlider_ValueChanged(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& e);

    private:
        // 현재 스트로크
        winrt::Microsoft::UI::Xaml::Shapes::Polyline m_currentStroke{ nullptr };
        bool m_isDrawing{ false };

        // 전역 펜 상태(새 스트로크에 적용)
        winrt::Windows::UI::Color m_currentColor{ 255, 0, 0, 0 }; // A,R,G,B (기본: 검정)
        double m_currentThickness{ 2.0 };

        // 모든 스트로크가 공유하는 공용 브러시(색 변경 즉시 반영)
        winrt::Microsoft::UI::Xaml::Media::SolidColorBrush m_currentBrush{ nullptr };
    };
}

namespace winrt::App1::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
```

```
#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Shapes.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Input.h>
#include <winrt/Microsoft.UI.Input.h>          // PointerPoint
#include <winrt/Microsoft.UI.Windowing.h>      // AppWindow (창 크기 조절 선택)

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Shapes;

namespace winrt::App1::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();

        // (선택) 창 크기 640x480
        if (auto aw = this->AppWindow())
        {
            aw.Resize({ 640, 480 });
        }

        // 공용 브러시 초기화 (현재 색으로)
        m_currentBrush = SolidColorBrush(m_currentColor);

        // ColorPicker 초기값을 코드에서도 맞춰 UI와 상태 일치
        if (auto cp = this->PenColorPicker())
        {
            cp.Color(m_currentColor);
        }

        // 굵기 텍스트 초기화
        if (auto tb = this->ThicknessValueText())
        {
            tb.Text(L"2.0 px");
        }
    }

    // ==== 툴바: 색상 선택 ====
    void MainWindow::PenColorPicker_ColorChanged(IInspectable const&,
        Microsoft::UI::Xaml::Controls::ColorChangedEventArgs const& e)
    {
        m_currentColor = e.NewColor();

        // 공유 브러시 색만 변경 → 현재/이후 스트로크 모두 즉시 반영
        if (m_currentBrush)
            m_currentBrush.Color(m_currentColor);

        // 선택 후 플라이아웃 닫기(원치 않으면 주석 처리)
        if (auto f = this->ColorFlyout())
            f.Hide();
    }

    // ==== 툴바: 굵기 선택 ====
    void MainWindow::ThicknessSlider_ValueChanged(IInspectable const&,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& e)
    {
        m_currentThickness = e.NewValue();

        // 현재 스트로크에 즉시 반영 (다음 스트로크도 이 값으로 시작)
        if (m_currentStroke)
            m_currentStroke.StrokeThickness(m_currentThickness);

        // 우측 표시 갱신
        if (auto tb = this->ThicknessValueText())
        {
            wchar_t buf[32]{};
            swprintf_s(buf, L"%.1f px", m_currentThickness);
            tb.Text(buf);
        }
    }

    // ==== 그리기 시작 ====
    void MainWindow::DrawCanvas_PointerPressed(IInspectable const&, PointerRoutedEventArgs const& e)
    {
        auto canvas = this->DrawCanvas();
        if (!canvas) return;

        auto point = e.GetCurrentPoint(canvas);
        if (!point.Properties().IsLeftButtonPressed()) return;

        m_isDrawing = true;

        m_currentStroke = Microsoft::UI::Xaml::Shapes::Polyline{};
        // 공유 브러시 사용(색 변경 즉시 반영)
        m_currentStroke.Stroke(m_currentBrush);
        m_currentStroke.StrokeThickness(m_currentThickness);
        m_currentStroke.StrokeLineJoin(PenLineJoin::Round);
        m_currentStroke.StrokeStartLineCap(PenLineCap::Round);
        m_currentStroke.StrokeEndLineCap(PenLineCap::Round);

        auto pos = point.Position();
        m_currentStroke.Points().Append(Windows::Foundation::Point{
            static_cast<float>(pos.X), static_cast<float>(pos.Y) });

        canvas.Children().Append(m_currentStroke);
        canvas.CapturePointer(e.Pointer());
        e.Handled(true);
    }

    // ==== 이동 중 점 추가 ====
    void MainWindow::DrawCanvas_PointerMoved(IInspectable const&, PointerRoutedEventArgs const& e)
    {
        if (!m_isDrawing || !m_currentStroke) return;

        auto canvas = this->DrawCanvas();
        if (!canvas) return;

        auto point = e.GetCurrentPoint(canvas);
        if (!point.IsInContact()) return;

        auto pos = point.Position();
        m_currentStroke.Points().Append(Windows::Foundation::Point{
            static_cast<float>(pos.X), static_cast<float>(pos.Y) });

        e.Handled(true);
    }

    // ==== 끝내기 ====
    void MainWindow::DrawCanvas_PointerReleased(IInspectable const&, PointerRoutedEventArgs const& e)
    {
        if (!m_isDrawing) return;

        if (auto canvas = this->DrawCanvas())
            canvas.ReleasePointerCaptures();

        m_isDrawing = false;
        m_currentStroke = nullptr;
        e.Handled(true);
    }
}

```



