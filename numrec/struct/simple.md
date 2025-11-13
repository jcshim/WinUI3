```
<?xml version="1.0" encoding="utf-8"?>
<Window
    x:Class="App3.MainWindow"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:local="using:App3"
    xmlns:d="http://schemas.microsoft.com/expression/blend/2008"
    xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006"
    mc:Ignorable="d"
    Title="App3">

    <!-- 2행 레이아웃: [0] 툴바 / [1] 그리기 영역 -->
    <Grid Width="1024" Height="800"
          Background="{ThemeResource ApplicationPageBackgroundThemeBrush}">
        <Grid.RowDefinitions>
            <RowDefinition Height="Auto"/>
            <RowDefinition Height="*"/>
        </Grid.RowDefinitions>

        <!-- 상단 도구막대 -->
        <StackPanel Grid.Row="0" Orientation="Horizontal" Spacing="12" Padding="8">

            <!-- 색상 버튼 + 플라이아웃(ColorPicker) -->
            <Button x:Name="ColorButton" VerticalAlignment="Center">
                <StackPanel Orientation="Horizontal" Spacing="8">
                    <TextBlock Text="색상" VerticalAlignment="Center"/>
                    <!-- 현재 색 미리보기 -->
                    <Ellipse x:Name="ColorPreview" Width="18" Height="18"
                             Stroke="{ThemeResource SystemControlForegroundBaseMediumBrush}"
                             StrokeThickness="1"/>
                </StackPanel>
                <Button.Flyout>
                    <Flyout x:Name="ColorFlyout" Placement="Bottom" ShouldConstrainToRootBounds="True">
                        <StackPanel Orientation="Vertical" Spacing="8" Padding="8">
                            <TextBlock Text="펜 색상 선택" Margin="0,0,0,4"/>
                            <ColorPicker x:Name="PenColorPicker"
                                         IsAlphaEnabled="False"
                                         IsMoreButtonVisible="False"
                                         MinWidth="220"
                                         Color="#FF000000"
                                         ColorChanged="PenColorPicker_ColorChanged"/>
                            <StackPanel Orientation="Horizontal" Spacing="8" HorizontalAlignment="Right">
                                <Button x:Name="ApplyColorButton" Content="적용" Click="ApplyColorButton_Click"/>
                                <Button Content="닫기" Click="CloseColorFlyout_Click"/>
                            </StackPanel>
                        </StackPanel>
                    </Flyout>
                </Button.Flyout>
            </Button>

            <!-- 굵기 슬라이더 -->
            <TextBlock Text="굵기" VerticalAlignment="Center" Margin="16,0,4,0"/>
            <Slider x:Name="ThicknessSlider"
                    Minimum="0.5" Maximum="24" StepFrequency="0.5"
                    Value="2"
                    Width="200"
                    ValueChanged="ThicknessSlider_ValueChanged"/>
            <TextBlock x:Name="ThicknessValueText"
                       VerticalAlignment="Center"
                       Text="2.0 px" Margin="6,0,0,0"/>

            <!-- 저장/불러오기/지우기/인식 -->
            <StackPanel Orientation="Horizontal" Spacing="8" Margin="16,0,0,0">
                <Button x:Name="SaveButton" Content="저장" Click="SaveButton_Click"/>
                <Button x:Name="LoadButton" Content="불러오기" Click="LoadButton_Click"/>
                <Button x:Name="ClearButton" Content="지우기" Click="ClearButton_Click"/>
                <Button x:Name="RecognizeButton" Content="인식" Click="RecognizeButton_Click"/>
            </StackPanel>
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

#include <vector>

#include <winrt/Windows.UI.h>                                // Color
#include <winrt/Windows.Foundation.h>                        // Point, IInspectable
#include <winrt/Microsoft.UI.Xaml.Shapes.h>
#include <winrt/Microsoft.UI.Xaml.Input.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>     // Slider ValueChanged args
#include <winrt/Microsoft.UI.Xaml.Media.h>

// JSON
#include <winrt/Windows.Data.Json.h>

// 파일 피커 (Win32 HWND 초기화용 인터페이스)
#include <windows.h>
#include <Shobjidl_core.h>            // IInitializeWithWindow
#include <microsoft.ui.xaml.window.h> // IWindowNative (WinUI 3 Window → HWND)

namespace winrt::App3::implementation
{
    struct StrokeData
    {
        winrt::Windows::UI::Color color{};
        double thickness{ 2.0 };
        std::vector<winrt::Windows::Foundation::Point> points{};
    };

    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();
        int32_t MyProperty();
        void MyProperty(int32_t value);

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
        void ApplyColorButton_Click(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void CloseColorFlyout_Click(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void ThicknessSlider_ValueChanged(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& e);

        // 저장/불러오기
        winrt::fire_and_forget SaveButton_Click(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        winrt::fire_and_forget LoadButton_Click(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        // 지우기 / 인식
        void ClearButton_Click(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        winrt::fire_and_forget RecognizeButton_Click(winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        // 현재 스트로크 핸들 및 데이터
        winrt::Microsoft::UI::Xaml::Shapes::Polyline m_currentStroke{ nullptr };
        StrokeData m_currentData{};
        bool m_isDrawing{ false };

        // 전역 펜 상태(다음 스트로크에 적용)
        winrt::Windows::UI::Color m_currentColor{ 255, 0, 0, 0 };
        winrt::Windows::UI::Color m_pendingColor{ 255, 0, 0, 0 };
        double m_currentThickness{ 2.0 };

        // 완료된 스트로크(문서 모델)
        std::vector<StrokeData> m_strokes;

        // 유틸
        void UpdateColorPreview();
        void RedrawFromModel();
        void InitializePickerWithWindow(winrt::Windows::Foundation::IInspectable const& picker);

        // 아주 단순한 구조적 숫자 인식 (0, 1, 또는 모름)
        winrt::hstring SimpleRecognizeDigit();

        // 오류 메시지 다이얼로그
        winrt::Windows::Foundation::IAsyncAction ShowErrorDialog(winrt::hstring const& message);
    };
}

namespace winrt::App3::factory_implementation
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
#include <winrt/Microsoft.UI.Input.h>                 // PointerPoint
#include <winrt/Microsoft.UI.Windowing.h>             // AppWindow

#include <winrt/Windows.Data.Json.h>                  // JSON
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Pickers.h>

#include <windows.h>
#include <Shobjidl_core.h>            // IInitializeWithWindow
#include <microsoft.ui.xaml.window.h> // IWindowNative

#include <sstream>
#include <cmath>                      // fabs

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Media;

using Windows::Data::Json::JsonArray;
using Windows::Data::Json::JsonObject;
using Windows::Data::Json::JsonValue;
using Windows::Foundation::IAsyncAction;

namespace winrt::App3::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();

        // 창 크기 1024x800
        if (auto aw = this->AppWindow())
        {
            aw.Resize({ 1024, 800 });
        }

        // UI 초기 동기화
        if (auto cp = this->PenColorPicker())
        {
            cp.Color(m_currentColor);
        }
        m_pendingColor = m_currentColor;
        UpdateColorPreview();

        if (auto tb = this->ThicknessValueText())
        {
            tb.Text(L"2.0 px");
        }
    }

    int32_t MainWindow::MyProperty() { throw hresult_not_implemented(); }
    void MainWindow::MyProperty(int32_t) { throw hresult_not_implemented(); }

    // ---- Win32 HWND로 FilePicker 초기화 (WinApp SDK 버전 무관) ----
    void MainWindow::InitializePickerWithWindow(Windows::Foundation::IInspectable const& picker)
    {
        // this(Window) -> IWindowNative -> HWND
        if (auto native = this->try_as<::IWindowNative>())
        {
            HWND hwnd{};
            if (SUCCEEDED(native->get_WindowHandle(&hwnd)) && hwnd)
            {
                if (auto init = picker.try_as<::IInitializeWithWindow>())
                {
                    (void)init->Initialize(hwnd);
                }
            }
        }
    }

    // ==== 오류 다이얼로그 ====
    IAsyncAction MainWindow::ShowErrorDialog(hstring const& message)
    {
        ContentDialog dlg{};
        dlg.XamlRoot(this->Content().XamlRoot());  // WinUI 3: XamlRoot 반드시 설정
        dlg.Title(box_value(L"오류"));
        dlg.Content(box_value(message));
        dlg.CloseButtonText(L"확인");
        co_await dlg.ShowAsync();
    }

    // ==== ColorPicker: 드래그 중 연속 호출 → 미리보기만 갱신 ====
    void MainWindow::PenColorPicker_ColorChanged(IInspectable const&,
        Microsoft::UI::Xaml::Controls::ColorChangedEventArgs const& e)
    {
        m_pendingColor = e.NewColor();
        UpdateColorPreview();
    }

    // ==== [적용] 버튼: 이때만 실제 펜 색 확정 ====
    void MainWindow::ApplyColorButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        m_currentColor = m_pendingColor;

        if (auto cp = this->PenColorPicker())
            cp.Color(m_currentColor);
        UpdateColorPreview();

        if (auto f = this->ColorFlyout())
            f.Hide();
    }

    void MainWindow::CloseColorFlyout_Click(IInspectable const&, RoutedEventArgs const&)
    {
        if (auto f = this->ColorFlyout())
            f.Hide();
    }

    // ==== 굵기 ====
    void MainWindow::ThicknessSlider_ValueChanged(IInspectable const&,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& e)
    {
        m_currentThickness = e.NewValue();

        if (auto tb = this->ThicknessValueText())
        {
            wchar_t buf[32]{};
            swprintf_s(buf, L"%.1f px", m_currentThickness);
            tb.Text(buf);
        }

        if (m_currentStroke)
            m_currentStroke.StrokeThickness(m_currentThickness);
    }

    // ==== 그리기 시작 ====
    void MainWindow::DrawCanvas_PointerPressed(IInspectable const&, PointerRoutedEventArgs const& e)
    {
        auto canvas = this->DrawCanvas();
        if (!canvas) return;

        auto point = e.GetCurrentPoint(canvas);
        if (!point.Properties().IsLeftButtonPressed()) return;

        m_isDrawing = true;

        Microsoft::UI::Xaml::Shapes::Polyline pl;
        pl.Stroke(Microsoft::UI::Xaml::Media::SolidColorBrush(m_currentColor));
        pl.StrokeThickness(m_currentThickness);
        pl.StrokeLineJoin(Microsoft::UI::Xaml::Media::PenLineJoin::Round);
        pl.StrokeStartLineCap(Microsoft::UI::Xaml::Media::PenLineCap::Round);
        pl.StrokeEndLineCap(Microsoft::UI::Xaml::Media::PenLineCap::Round);

        auto pos = point.Position();
        Windows::Foundation::Point p{ static_cast<float>(pos.X), static_cast<float>(pos.Y) };
        pl.Points().Append(p);

        canvas.Children().Append(pl);
        canvas.CapturePointer(e.Pointer());

        m_currentStroke = pl;

        m_currentData = {};
        m_currentData.color = m_currentColor;
        m_currentData.thickness = m_currentThickness;
        m_currentData.points.push_back(p);

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
        Windows::Foundation::Point p{ static_cast<float>(pos.X), static_cast<float>(pos.Y) };
        m_currentStroke.Points().Append(p);
        m_currentData.points.push_back(p);

        e.Handled(true);
    }

    // ==== 끝내기 ====
    void MainWindow::DrawCanvas_PointerReleased(IInspectable const&, PointerRoutedEventArgs const& e)
    {
        if (!m_isDrawing) return;

        if (auto canvas = this->DrawCanvas())
            canvas.ReleasePointerCaptures();

        m_isDrawing = false;

        if (!m_currentData.points.empty())
            m_strokes.push_back(std::move(m_currentData));

        m_currentStroke = nullptr;
        e.Handled(true);
    }

    // ==== 저장 (파일 이름/경로 선택, 안정화) ====
    winrt::fire_and_forget MainWindow::SaveButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        hstring err;   // catch에서 메시지 저장 (catch 안에서 co_await 금지)
        try
        {
            // 문서 JSON 구성
            JsonArray rootArray;
            for (auto const& s : m_strokes)
            {
                JsonObject o;

                JsonArray c;
                c.Append(JsonValue::CreateNumberValue(s.color.A));
                c.Append(JsonValue::CreateNumberValue(s.color.R));
                c.Append(JsonValue::CreateNumberValue(s.color.G));
                c.Append(JsonValue::CreateNumberValue(s.color.B));
                o.Insert(L"color", c);

                o.Insert(L"thickness", JsonValue::CreateNumberValue(s.thickness));

                JsonArray pts;
                for (auto const& pt : s.points)
                {
                    JsonObject jp;
                    jp.Insert(L"x", JsonValue::CreateNumberValue(pt.X));
                    jp.Insert(L"y", JsonValue::CreateNumberValue(pt.Y));
                    pts.Append(jp);
                }
                o.Insert(L"points", pts);

                rootArray.Append(o);
            }

            JsonObject doc;
            doc.Insert(L"version", JsonValue::CreateNumberValue(1));
            doc.Insert(L"strokes", rootArray);

            Windows::Storage::Pickers::FileSavePicker picker;
            InitializePickerWithWindow(picker);
            picker.SuggestedStartLocation(Windows::Storage::Pickers::PickerLocationId::DocumentsLibrary);
            picker.SuggestedFileName(L"strokes");
            picker.FileTypeChoices().Insert(L"JSON", single_threaded_vector<hstring>({ L".json" }));

            Windows::Storage::StorageFile file = co_await picker.PickSaveFileAsync();
            if (!file) co_return; // 취소

            co_await Windows::Storage::FileIO::WriteTextAsync(file, doc.Stringify());
        }
        catch (winrt::hresult_error const& ex)
        {
            err = L"파일 저장 중 예외가 발생했습니다.\n\n" + ex.message();
        }
        catch (...)
        {
            err = L"파일 저장 중 알 수 없는 오류가 발생했습니다.";
        }

        if (!err.empty())
        {
            co_await ShowErrorDialog(err);
        }
        co_return;
    }

    // ==== 불러오기 (파일 선택, 방어적 파싱) ====
    winrt::fire_and_forget MainWindow::LoadButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        hstring err;   // catch에서 메시지 저장
        try
        {
            Windows::Storage::Pickers::FileOpenPicker picker;
            InitializePickerWithWindow(picker);
            picker.SuggestedStartLocation(Windows::Storage::Pickers::PickerLocationId::DocumentsLibrary);
            picker.FileTypeFilter().Append(L".json");

            Windows::Storage::StorageFile file = co_await picker.PickSingleFileAsync();
            if (!file) co_return; // 취소

            auto text = co_await Windows::Storage::FileIO::ReadTextAsync(file);

            JsonObject doc;
            if (!JsonObject::TryParse(text, doc))
            {
                err = L"JSON 파싱 실패: 올바른 형식의 파일인지 확인하세요.";
            }
            else
            {
                // strokes 키 확인
                if (!doc.HasKey(L"strokes"))
                {
                    err = L"JSON에 'strokes' 키가 없습니다.";
                }
                else if (doc.Lookup(L"strokes").ValueType() != Windows::Data::Json::JsonValueType::Array)
                {
                    err = L"'strokes' 값이 배열(JSON array)이 아닙니다.";
                }
                else
                {
                    auto arr = doc.Lookup(L"strokes").GetArray();

                    // 모델 갱신
                    m_strokes.clear();

                    for (uint32_t i = 0; i < arr.Size(); ++i)
                    {
                        auto itemVal = arr.GetAt(i);
                        if (itemVal.ValueType() != Windows::Data::Json::JsonValueType::Object)
                            continue;

                        auto o = itemVal.GetObject();
                        StrokeData s{};

                        // color
                        if (o.HasKey(L"color") && o.Lookup(L"color").ValueType() == Windows::Data::Json::JsonValueType::Array)
                        {
                            auto c = o.Lookup(L"color").GetArray();
                            if (c.Size() == 4)
                            {
                                s.color = {
                                    static_cast<uint8_t>(c.GetAt(0).GetNumber()),
                                    static_cast<uint8_t>(c.GetAt(1).GetNumber()),
                                    static_cast<uint8_t>(c.GetAt(2).GetNumber()),
                                    static_cast<uint8_t>(c.GetAt(3).GetNumber())
                                };
                            }
                        }

                        // thickness
                        if (o.HasKey(L"thickness") && o.Lookup(L"thickness").ValueType() == Windows::Data::Json::JsonValueType::Number)
                        {
                            s.thickness = o.Lookup(L"thickness").GetNumber();
                        }

                        // points
                        if (o.HasKey(L"points") && o.Lookup(L"points").ValueType() == Windows::Data::Json::JsonValueType::Array)
                        {
                            auto pts = o.Lookup(L"points").GetArray();
                            s.points.reserve(pts.Size());
                            for (uint32_t k = 0; k < pts.Size(); ++k)
                            {
                                auto pv = pts.GetAt(k);
                                if (pv.ValueType() != Windows::Data::Json::JsonValueType::Object)
                                    continue;

                                auto jp = pv.GetObject();
                                if (jp.HasKey(L"x") && jp.HasKey(L"y") &&
                                    jp.Lookup(L"x").ValueType() == Windows::Data::Json::JsonValueType::Number &&
                                    jp.Lookup(L"y").ValueType() == Windows::Data::Json::JsonValueType::Number)
                                {
                                    float x = static_cast<float>(jp.Lookup(L"x").GetNumber());
                                    float y = static_cast<float>(jp.Lookup(L"y").GetNumber());
                                    s.points.push_back({ x, y });
                                }
                            }
                        }

                        if (!s.points.empty())
                            m_strokes.push_back(std::move(s));
                    }

                    // UI 갱신
                    RedrawFromModel();
                }
            }
        }
        catch (winrt::hresult_error const& ex)
        {
            err = L"파일 읽기 중 예외가 발생했습니다.\n\n" + ex.message();
        }
        catch (...)
        {
            err = L"파일 읽기 중 알 수 없는 오류가 발생했습니다.";
        }

        if (!err.empty())
        {
            co_await ShowErrorDialog(err);
        }
        co_return;
    }

    // ==== 색 미리보기 칩 ====
    void MainWindow::UpdateColorPreview()
    {
        if (auto chip = this->ColorPreview())
        {
            chip.Fill(Microsoft::UI::Xaml::Media::SolidColorBrush(m_pendingColor));
        }
    }

    // ==== 모델 → 캔버스 재구성 ====
    void MainWindow::RedrawFromModel()
    {
        auto canvas = this->DrawCanvas();
        if (!canvas) return;

        canvas.Children().Clear();

        for (auto const& s : m_strokes)
        {
            if (s.points.empty()) continue;

            Microsoft::UI::Xaml::Shapes::Polyline pl;
            pl.Stroke(Microsoft::UI::Xaml::Media::SolidColorBrush(s.color));
            pl.StrokeThickness(s.thickness);
            pl.StrokeLineJoin(Microsoft::UI::Xaml::Media::PenLineJoin::Round);
            pl.StrokeStartLineCap(Microsoft::UI::Xaml::Media::PenLineCap::Round);
            pl.StrokeEndLineCap(Microsoft::UI::Xaml::Media::PenLineCap::Round);

            for (auto const& pt : s.points)
            {
                pl.Points().Append(pt);
            }

            canvas.Children().Append(pl);
        }
    }

    // ==== [지우기] 버튼: 화면 및 모델 초기화 ====
    void MainWindow::ClearButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        m_strokes.clear();
        m_currentData = {};
        m_currentStroke = nullptr;
        m_isDrawing = false;

        if (auto canvas = this->DrawCanvas())
        {
            canvas.Children().Clear();
        }
    }

    // ==== 아주 단순한 구조적 숫자 인식 ====
    // - 획 개수
    // - 전체 박스 폭/높이(aspect ratio)
    // - 첫 획 시작점/끝점 위치, X차이/거리
    // 규칙:
    //   * 1개의 긴 세로 획, 시작/끝 X가 거의 같으면 -> '1'
    //   * 1개의 닫힌 곡선(시작/끝 점이 가깝고 폭,높이가 비슷하면) -> '0'
    //   * 나머지 -> '?'
    // ==== 아주 단순한 구조적 숫자 인식 ====
// - 획 개수
// - 전체 박스 폭/높이(aspect ratio)
// - 첫 획 시작점/끝점 위치, X차이/거리
// 규칙:
//   * 1개의 긴 세로 획, 시작/끝 X가 거의 같으면 -> '1'
//   * 1개의 닫힌 곡선(시작/끝 점이 가깝고 폭,높이가 비슷하면) -> '0'
//   * 나머지 -> '?'
    hstring MainWindow::SimpleRecognizeDigit()
    {
        if (m_strokes.empty())
        {
            return L"획(스트로크)이 없습니다.\n숫자를 먼저 그려 주세요.";
        }

        // 전체 포인트 모으기 (std::min / std::max 사용 안 함)
        float minX = 1e9f, minY = 1e9f;
        float maxX = -1e9f, maxY = -1e9f;
        uint32_t totalPoints = 0;

        for (auto const& s : m_strokes)
        {
            for (auto const& p : s.points)
            {
                if (p.X < minX) minX = p.X;
                if (p.Y < minY) minY = p.Y;
                if (p.X > maxX) maxX = p.X;
                if (p.Y > maxY) maxY = p.Y;
                totalPoints++;
            }
        }

        if (totalPoints == 0)
        {
            return L"포인트가 없습니다.\n획이 올바르게 저장되지 않았습니다.";
        }

        double width = static_cast<double>(maxX - minX);
        double height = static_cast<double>(maxY - minY);
        if (width < 1.0 || height < 1.0)
        {
            return L"너무 작은 점/선입니다.\n조금 더 크게 숫자를 써 주세요.";
        }

        uint32_t strokeCount = static_cast<uint32_t>(m_strokes.size());

        // 첫 번째 획의 시작/끝 사용
        auto const& s0 = m_strokes.front();
        auto const& start = s0.points.front();
        auto const& end = s0.points.back();

        double dx = std::fabs(static_cast<double>(start.X - end.X));
        double dy = std::fabs(static_cast<double>(start.Y - end.Y));
        double aspect = height / width; // 세로/가로 비율

        // 시작/끝 점 거리 (닫힌 곡선 여부 대충 체크)
        double distStartEnd = std::sqrt(dx * dx + dy * dy);

        wchar_t      digit = L'?';
        std::wstring reason = L"";

        if (strokeCount == 1)
        {
            // 세로로 긴 한 획: 1
            if (aspect > 1.5 && dx < width * 0.3)
            {
                digit = L'1';
                reason = L"세로로 길고, 시작/끝 X 위치가 비슷해서 1로 추정했습니다.";
            }
            // 폭, 높이 비슷 + 시작/끝 점 가까움: 0
            else if (aspect > 0.7 && aspect < 1.4 &&
                distStartEnd < (width + height) * 0.25)
            {
                digit = L'0';
                reason = L"폭과 높이가 비슷하고, 시작점과 끝점이 가까워서 "
                    L"0(닫힌 곡선)에 가깝다고 보았습니다.";
            }
            else
            {
                digit = L'?';
                reason = L"단일 획이지만, 간단한 규칙으로는 0이나 1로 보기 애매합니다.";
            }
        }
        else
        {
            digit = L'?';
            reason = L"획이 여러 개라서(복수 스트로크), "
                L"이 예제의 단순 규칙으로는 숫자를 결정하지 않습니다.";
        }

        // 설명 문자열 만들기
        std::wstringstream ss;
        ss << L"[구조적 특징]\n";
        ss << L"획 개수: " << strokeCount << L"\n";
        ss << L"전체 박스 폭: " << width << L", 높이: " << height << L"\n";
        ss << L"세로/가로 비: " << aspect << L"\n\n";
        ss << L"첫 획 시작점: (" << start.X << L", " << start.Y << L")\n";
        ss << L"첫 획 끝점  : (" << end.X << L", " << end.Y << L")\n";
        ss << L"시작-끝 X 차이: " << dx << L"\n";
        ss << L"시작-끝 Y 차이: " << dy << L"\n";
        ss << L"시작-끝 거리  : " << distStartEnd << L"\n\n";

        ss << L"[판단]\n";
        if (digit == L'?')
        {
            ss << L"예상 숫자: (결정 불가)\n";
        }
        else
        {
            ss << L"예상 숫자: " << digit << L"\n";
        }
        ss << reason << L"\n\n";
        ss << L"※ 이 예제는 NN 없이, 시작점/끝점/박스 크기만 이용하는\n"
            << L"   아주 단순한 \"구조적 인식\" 예시입니다.\n"
            << L"   실제 인식률은 높지 않고, 교육용으로만 사용하세요.";

        return hstring{ ss.str() };
    }

    // ==== [인식] 버튼: 구조적 분석 결과를 다이얼로그로 표시 ====
    winrt::fire_and_forget MainWindow::RecognizeButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        auto result = SimpleRecognizeDigit();

        ContentDialog dlg{};
        dlg.XamlRoot(this->Content().XamlRoot());
        dlg.Title(box_value(L"인식 결과 (구조적 분석 데모)"));
        dlg.Content(box_value(result));
        dlg.CloseButtonText(L"확인");

        co_await dlg.ShowAsync();
        co_return;
    }
}
```
