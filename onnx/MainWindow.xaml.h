#pragma once
#include "MainWindow.g.h"

#include <vector>
#include <cmath>
#include <string_view>
#include <cstdint>

// WinUI / C++/WinRT
#include <winrt/Windows.UI.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.UI.Xaml.Shapes.h>
#include <winrt/Microsoft.UI.Xaml.Input.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Storage.Streams.h>

// File pickers HWND init
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#include <windows.h>
#include <Shobjidl_core.h>
#include <microsoft.ui.xaml.window.h>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// ONNX Runtime
#include <onnxruntime_cxx_api.h>

namespace winrt::App1::implementation
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

        // Drawing
        void DrawCanvas_PointerPressed(winrt::Windows::Foundation::IInspectable const&,
            winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
        void DrawCanvas_PointerMoved(winrt::Windows::Foundation::IInspectable const&,
            winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
        void DrawCanvas_PointerReleased(winrt::Windows::Foundation::IInspectable const&,
            winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);

        // Toolbar
        void PenColorPicker_ColorChanged(winrt::Windows::Foundation::IInspectable const&,
            winrt::Microsoft::UI::Xaml::Controls::ColorChangedEventArgs const& e);
        void ApplyColorButton_Click(winrt::Windows::Foundation::IInspectable const&,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void CloseColorFlyout_Click(winrt::Windows::Foundation::IInspectable const&,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void ThicknessSlider_ValueChanged(winrt::Windows::Foundation::IInspectable const&,
            winrt::Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& e);

        // File I/O
        winrt::fire_and_forget SaveButton_Click(winrt::Windows::Foundation::IInspectable const&,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        winrt::fire_and_forget LoadButton_Click(winrt::Windows::Foundation::IInspectable const&,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void ClearButton_Click(winrt::Windows::Foundation::IInspectable const&,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        // 숫자인식 (ONNX Runtime)
        winrt::fire_and_forget RecognizeButton_Click(winrt::Windows::Foundation::IInspectable const&,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        // drawing state
        winrt::Microsoft::UI::Xaml::Shapes::Polyline m_currentStroke{ nullptr };
        StrokeData m_currentData{};
        bool m_isDrawing{ false };

        // pen state
        winrt::Windows::UI::Color m_currentColor{ 255, 0, 0, 0 };
        winrt::Windows::UI::Color m_pendingColor{ 255, 0, 0, 0 };
        double m_currentThickness{ 2.0 };

        // strokes model
        std::vector<StrokeData> m_strokes;

        // UI apartment capture
        winrt::apartment_context m_ui;

        // ui helpers
        void UpdateColorPreview();
        void RedrawFromModel();
        void InitializePickerWithWindow(winrt::Windows::Foundation::IInspectable const& picker);
        winrt::Windows::Foundation::IAsyncAction ShowErrorDialog(winrt::hstring const& message);

        // canvas -> MNIST tensor (1x1x28x28, float32) with advanced preprocessing
        winrt::Windows::Foundation::IAsyncOperation<bool>
            TryRenderToMnistTensor(std::vector<float>& outTensor28x28);
    };
}

namespace winrt::App1::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
