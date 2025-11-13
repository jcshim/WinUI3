#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

// WinUI
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Shapes.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Input.h>
#include <winrt/Microsoft.UI.Input.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>

#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Storage.Streams.h>

#include <windows.h>
#include <Shobjidl_core.h>
#include <microsoft.ui.xaml.window.h>

#include <sstream>
#include <algorithm>
#include <array>
#include <limits>
#include <numeric>
#include <cmath>

// ONNX Runtime
#include <onnxruntime_cxx_api.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;

using Windows::Data::Json::JsonArray;
using Windows::Data::Json::JsonObject;
using Windows::Data::Json::JsonValue;
using Windows::Foundation::IAsyncAction;

namespace
{
    // ONNX Runtime Env는 한 번만 초기화
    Ort::Env& GetOrtEnv()
    {
        static Ort::Env env{ ORT_LOGGING_LEVEL_WARNING, "App1" };
        return env;
    }

    // BGRA -> (반전) 그레이스케일 [0..1]
    inline float inv_gray_from_BGRA(uint8_t B, uint8_t G, uint8_t R)
    {
        float gray = 0.2126f * R + 0.7152f * G + 0.0722f * B; // ITU-R BT.709
        float v = (255.0f - gray) / 255.0f;                  // 배경 0, 선 1
        if (v < 0.f) v = 0.f;
        if (v > 1.f) v = 1.f;
        return v;
    }

    // 외접 박스 계산: 값이 thresh 이상인 픽셀만 "그림"으로 간주
    inline bool compute_bbox(const std::vector<uint8_t>& src, int w, int h,
        float thresh, int& x0, int& y0, int& x1, int& y1)
    {
        x0 = y0 = (std::numeric_limits<int>::max)();
        x1 = y1 = (std::numeric_limits<int>::min)();
        bool found = false;

        for (int y = 0; y < h; ++y)
        {
            const size_t row = static_cast<size_t>(y) * w * 4;
            for (int x = 0; x < w; ++x)
            {
                size_t idx = row + static_cast<size_t>(x) * 4;
                float v = inv_gray_from_BGRA(src[idx + 0], src[idx + 1], src[idx + 2]);
                if (v >= thresh)
                {
                    found = true;
                    if (x < x0) x0 = x;
                    if (x > x1) x1 = x;
                    if (y < y0) y0 = y;
                    if (y > y1) y1 = y;
                }
            }
        }
        return found;
    }

    // 28×28 bilinear 리샘플 (원본은 BGRA8, [0..1] 반전 그레이로 변환)
    inline void resample_bilinear_28_from_bgra(
        const std::vector<uint8_t>& src, int w, int h,
        float sx0, float sy0, float side,
        std::vector<float>& out28)
    {
        const int D = 28;
        out28.assign(D * D, 0.0f);
        if (side <= 1.f) side = 1.f;

        const float scale = side / static_cast<float>(D); // src pixels per 1 output pixel
        for (int oy = 0; oy < D; ++oy)
        {
            float sy = sy0 + (oy + 0.5f) * scale - 0.5f;
            int   y0 = static_cast<int>(std::floor(sy));
            float wy = sy - y0;

            if (y0 < 0) { y0 = 0; wy = 0.f; }
            if (y0 >= h - 1) { y0 = h - 2; wy = 1.f; }

            for (int ox = 0; ox < D; ++ox)
            {
                float sx = sx0 + (ox + 0.5f) * scale - 0.5f;
                int   x0 = static_cast<int>(std::floor(sx));
                float wx = sx - x0;

                if (x0 < 0) { x0 = 0; wx = 0.f; }
                if (x0 >= w - 1) { x0 = w - 2; wx = 1.f; }

                auto sample = [&](int px, int py) -> float
                    {
                        size_t idx = (static_cast<size_t>(py) * w + px) * 4;
                        return inv_gray_from_BGRA(src[idx + 0], src[idx + 1], src[idx + 2]);
                    };

                float v00 = sample(x0, y0);
                float v10 = sample(x0 + 1, y0);
                float v01 = sample(x0, y0 + 1);
                float v11 = sample(x0 + 1, y0 + 1);

                float v0 = v00 + wx * (v10 - v00);
                float v1 = v01 + wx * (v11 - v01);
                float v = v0 + wy * (v1 - v0);

                out28[oy * D + ox] = std::clamp(v, 0.f, 1.f);
            }
        }
    }

    // 가우시안 블러 (5x5, sigma≈1.0) — separable
    inline void gaussian_blur_5x5(std::vector<float>& img28)
    {
        const int D = 28;
        static const float k[5] = { 0.06136f, 0.24477f, 0.38774f, 0.24477f, 0.06136f }; // 정규화됨
        std::vector<float> tmp(img28.size(), 0.f);

        // horizontal
        for (int y = 0; y < D; ++y)
        {
            for (int x = 0; x < D; ++x)
            {
                float acc = 0.f;
                for (int t = -2; t <= 2; ++t)
                {
                    int xx = std::clamp(x + t, 0, D - 1);
                    acc += img28[y * D + xx] * k[t + 2];
                }
                tmp[y * D + x] = acc;
            }
        }
        // vertical
        for (int y = 0; y < D; ++y)
        {
            for (int x = 0; x < D; ++x)
            {
                float acc = 0.f;
                for (int t = -2; t <= 2; ++t)
                {
                    int yy = std::clamp(y + t, 0, D - 1);
                    acc += tmp[yy * D + x] * k[t + 2];
                }
                img28[y * D + x] = acc;
            }
        }
    }

    // Otsu 임계값 계산 (입력 [0..1])
    inline float otsu_threshold_0_1(const std::vector<float>& img28)
    {
        const int bins = 256;
        std::array<int, bins> hist{}; hist.fill(0);
        for (float v : img28)
        {
            int b = static_cast<int>(std::round(std::clamp(v, 0.f, 1.f) * 255.f));
            hist[b]++;
        }
        const int N = static_cast<int>(img28.size());
        double sumAll = 0.0;
        for (int i = 0; i < bins; ++i) sumAll += i * hist[i];

        int wB = 0;        // weight background
        int wF = 0;        // weight foreground
        double sumB = 0.0; // sum background
        double maxVar = -1.0;
        int bestT = 128;

        for (int t = 0; t < bins; ++t)
        {
            wB += hist[t];
            if (wB == 0) continue;
            wF = N - wB;
            if (wF == 0) break;

            sumB += t * hist[t];
            double mB = sumB / wB;
            double mF = (sumAll - sumB) / wF;

            double between = static_cast<double>(wB) * wF * (mB - mF) * (mB - mF);
            if (between > maxVar) { maxVar = between; bestT = t; }
        }
        return static_cast<float>(bestT / 255.0);
    }

    // 질량중심(무게중심) 계산 (mask 또는 그레이스케일)
    inline std::pair<double, double> centroid(const std::vector<float>& img28)
    {
        const int D = 28;
        double sum = 0.0, mx = 0.0, my = 0.0;
        for (int y = 0; y < D; ++y)
        {
            for (int x = 0; x < D; ++x)
            {
                double v = img28[y * D + x];
                sum += v;
                mx += v * x;
                my += v * y;
            }
        }
        if (sum <= 1e-8) return { 13.5, 13.5 }; // 중앙
        return { mx / sum, my / sum };
    }

    // 28×28 평행이동(서브픽셀, bilinear 샘플링)
    inline void translate_bilinear_28(const std::vector<float>& src, double dx, double dy, std::vector<float>& dst)
    {
        const int D = 28;
        dst.assign(D * D, 0.f);
        for (int y = 0; y < D; ++y)
        {
            double sy = y - dy;
            int y0 = static_cast<int>(std::floor(sy));
            double wy = sy - y0;

            if (y0 < 0) { y0 = 0; wy = 0.0; }
            if (y0 >= D - 1) { y0 = D - 2; wy = 1.0; }

            for (int x = 0; x < D; ++x)
            {
                double sx = x - dx;
                int x0 = static_cast<int>(std::floor(sx));
                double wx = sx - x0;

                if (x0 < 0) { x0 = 0; wx = 0.0; }
                if (x0 >= D - 1) { x0 = D - 2; wx = 1.0; }

                auto smp = [&](int px, int py) -> float { return src[py * D + px]; };

                double v00 = smp(x0, y0);
                double v10 = smp(x0 + 1, y0);
                double v01 = smp(x0, y0 + 1);
                double v11 = smp(x0 + 1, y0 + 1);

                double v0 = v00 + wx * (v10 - v00);
                double v1 = v01 + wx * (v11 - v01);
                double v = v0 + wy * (v1 - v0);

                dst[y * D + x] = static_cast<float>(std::clamp(v, 0.0, 1.0));
            }
        }
    }

    // z-score 표준화 (전체 픽셀 단위)
    inline void zscore_normalize(std::vector<float>& img28, float mean, float stddev)
    {
        if (stddev <= 1e-6f) stddev = 1.f;
        for (auto& v : img28) v = (v - mean) / stddev;
    }
}

namespace winrt::App1::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();
        m_ui = winrt::apartment_context{}; // UI apartment capture

        if (auto aw = this->AppWindow())
            aw.Resize({ 1024, 800 });

        if (auto cp = this->PenColorPicker())
            cp.Color(m_currentColor);

        UpdateColorPreview();

        if (auto tb = this->ThicknessValueText())
            tb.Text(L"2.0 px");
    }
    int32_t MainWindow::MyProperty() { throw hresult_not_implemented(); }
    void MainWindow::MyProperty(int32_t) { throw hresult_not_implemented(); }

    void MainWindow::InitializePickerWithWindow(Windows::Foundation::IInspectable const& picker)
    {
        if (auto native = this->try_as<::IWindowNative>())
        {
            HWND hwnd{};
            if (SUCCEEDED(native->get_WindowHandle(&hwnd)) && hwnd)
            {
                if (auto init = picker.try_as<::IInitializeWithWindow>())
                    (void)init->Initialize(hwnd);
            }
        }
    }

    IAsyncAction MainWindow::ShowErrorDialog(hstring const& message)
    {
        co_await m_ui; // UI thread
        ContentDialog dlg{};
        dlg.XamlRoot(this->Content().XamlRoot());
        dlg.Title(box_value(L"오류"));
        dlg.Content(box_value(message));
        dlg.CloseButtonText(L"확인");
        co_await dlg.ShowAsync();
    }

    void MainWindow::PenColorPicker_ColorChanged(IInspectable const&,
        Microsoft::UI::Xaml::Controls::ColorChangedEventArgs const& e)
    {
        m_pendingColor = e.NewColor();
        UpdateColorPreview();
    }

    void MainWindow::ApplyColorButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        m_currentColor = m_pendingColor;
        if (auto cp = this->PenColorPicker()) cp.Color(m_currentColor);
        UpdateColorPreview();
        if (auto f = this->ColorFlyout()) f.Hide();
    }

    void MainWindow::CloseColorFlyout_Click(IInspectable const&, RoutedEventArgs const&)
    {
        if (auto f = this->ColorFlyout()) f.Hide();
    }

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
        if (m_currentStroke) m_currentStroke.StrokeThickness(m_currentThickness);
    }

    void MainWindow::DrawCanvas_PointerPressed(IInspectable const&, PointerRoutedEventArgs const& e)
    {
        auto canvas = this->DrawCanvas();
        if (!canvas) return;

        auto point = e.GetCurrentPoint(canvas);
        if (!point.Properties().IsLeftButtonPressed()) return;

        m_isDrawing = true;

        Microsoft::UI::Xaml::Shapes::Polyline pl;
        pl.Stroke(SolidColorBrush(m_currentColor));
        pl.StrokeThickness(m_currentThickness);
        pl.StrokeLineJoin(PenLineJoin::Round);
        pl.StrokeStartLineCap(PenLineCap::Round);
        pl.StrokeEndLineCap(PenLineCap::Round);

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

    winrt::fire_and_forget MainWindow::SaveButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        hstring err;
        try
        {
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
            if (!file) co_return;

            co_await Windows::Storage::FileIO::WriteTextAsync(file, doc.Stringify());
        }
        catch (winrt::hresult_error const& ex)
        {
            hstring msg = ex.message();
            err = L"파일 저장 중 예외가 발생했습니다.\n\n" + msg;
        }
        catch (...)
        {
            err = L"파일 저장 중 알 수 없는 오류가 발생했습니다.";
        }

        if (!err.empty()) co_await ShowErrorDialog(err);
        co_return;
    }

    winrt::fire_and_forget MainWindow::LoadButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        hstring err;
        try
        {
            Windows::Storage::Pickers::FileOpenPicker picker;
            InitializePickerWithWindow(picker);
            picker.SuggestedStartLocation(Windows::Storage::Pickers::PickerLocationId::DocumentsLibrary);
            picker.FileTypeFilter().Append(L".json");

            Windows::Storage::StorageFile file = co_await picker.PickSingleFileAsync();
            if (!file) co_return;

            auto text = co_await Windows::Storage::FileIO::ReadTextAsync(file);

            JsonObject doc;
            if (!JsonObject::TryParse(text, doc)) { err = L"JSON 파싱 실패: 올바른 형식인지 확인하세요."; }
            else
            {
                if (!doc.HasKey(L"strokes")) { err = L"JSON에 'strokes' 키가 없습니다."; }
                else if (doc.Lookup(L"strokes").ValueType() != Windows::Data::Json::JsonValueType::Array) { err = L"'strokes' 값이 배열이 아닙니다."; }
                else
                {
                    auto arr = doc.Lookup(L"strokes").GetArray();
                    m_strokes.clear();

                    for (uint32_t i = 0; i < arr.Size(); ++i)
                    {
                        auto itemVal = arr.GetAt(i);
                        if (itemVal.ValueType() != Windows::Data::Json::JsonValueType::Object) continue;

                        auto o = itemVal.GetObject();
                        StrokeData s{};

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

                        if (o.HasKey(L"thickness") && o.Lookup(L"thickness").ValueType() == Windows::Data::Json::JsonValueType::Number)
                            s.thickness = o.Lookup(L"thickness").GetNumber();

                        if (o.HasKey(L"points") && o.Lookup(L"points").ValueType() == Windows::Data::Json::JsonValueType::Array)
                        {
                            auto pts = o.Lookup(L"points").GetArray();
                            s.points.reserve(pts.Size());
                            for (uint32_t k = 0; k < pts.Size(); ++k)
                            {
                                auto pv = pts.GetAt(k);
                                if (pv.ValueType() != Windows::Data::Json::JsonValueType::Object) continue;
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
                        if (!s.points.empty()) m_strokes.push_back(std::move(s));
                    }

                    co_await m_ui; // UI
                    RedrawFromModel();
                }
            }
        }
        catch (winrt::hresult_error const& ex)
        {
            hstring msg = ex.message();
            err = L"파일 읽기 중 예외가 발생했습니다。\n\n" + msg;
        }
        catch (...)
        {
            err = L"파일 읽기 중 알 수 없는 오류가 발생했습니다.";
        }

        if (!err.empty()) co_await ShowErrorDialog(err);
        co_return;
    }

    void MainWindow::UpdateColorPreview()
    {
        if (auto chip = this->ColorPreview())
            chip.Fill(SolidColorBrush(m_pendingColor));
    }

    void MainWindow::RedrawFromModel()
    {
        auto canvas = this->DrawCanvas();
        if (!canvas) return;

        canvas.Children().Clear();

        for (auto const& s : m_strokes)
        {
            if (s.points.empty()) continue;

            Microsoft::UI::Xaml::Shapes::Polyline pl;
            pl.Stroke(SolidColorBrush(s.color));
            pl.StrokeThickness(s.thickness);
            pl.StrokeLineJoin(PenLineJoin::Round);
            pl.StrokeStartLineCap(PenLineCap::Round);
            pl.StrokeEndLineCap(PenLineCap::Round);

            for (auto const& pt : s.points) pl.Points().Append(pt);

            canvas.Children().Append(pl);
        }
    }

    void MainWindow::ClearButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        if (auto canvas = this->DrawCanvas())
        {
            canvas.ReleasePointerCaptures();
            canvas.Children().Clear();
        }
        m_isDrawing = false;
        m_currentStroke = nullptr;
        m_currentData = {};
        m_strokes.clear();
    }

    // ----- 캔버스 → 28x28 float 텐서 (전처리 포함) -----
    Windows::Foundation::IAsyncOperation<bool>
        MainWindow::TryRenderToMnistTensor(std::vector<float>& outTensor28x28)
    {
        auto canvas = this->DrawCanvas();
        if (!canvas) co_return false;

        // UI 스레드에서 렌더
        co_await m_ui;

        RenderTargetBitmap rtb;
        co_await rtb.RenderAsync(canvas);

        const int srcW = rtb.PixelWidth();
        const int srcH = rtb.PixelHeight();
        if (srcW <= 0 || srcH <= 0) co_return false;

        auto ibuf = co_await rtb.GetPixelsAsync(); // BGRA8
        auto reader = Windows::Storage::Streams::DataReader::FromBuffer(ibuf);
        std::vector<uint8_t> src(ibuf.Length());
        reader.ReadBytes(array_view<uint8_t>(src));

        // 1) 외접 박스
        int bx0, by0, bx1, by1;
        constexpr float THRESH = 0.05f; // 5% 이상만 "획"으로 간주
        bool found = compute_bbox(src, srcW, srcH, THRESH, bx0, by0, bx1, by1);

        // 2) 외접 박스 없으면 전체 프레임 리샘플
        float sx0 = 0.f, sy0 = 0.f, side = static_cast<float>(std::min(srcW, srcH));
        if (found)
        {
            const float bw = static_cast<float>(bx1 - bx0 + 1);
            const float bh = static_cast<float>(by1 - by0 + 1);
            const float longSide = (bw > bh ? bw : bh);
            const float pad = 4.0f; // 여백
            side = longSide + 2.f * pad;

            // 중심 기준 정사각 좌상단
            const float cx = (bx0 + bx1) * 0.5f;
            const float cy = (by0 + by1) * 0.5f;
            sx0 = cx - side * 0.5f;
            sy0 = cy - side * 0.5f;

            // 경계 클램프
            if (sx0 < 0.f) sx0 = 0.f;
            if (sy0 < 0.f) sy0 = 0.f;
            if (sx0 + side > srcW) side = srcW - sx0;
            if (sy0 + side > srcH) side = srcH - sy0;
            if (side <= 1.f) side = 1.f;
        }

        // 3) 28×28 bilinear 리샘플 (반전 그레이)
        std::vector<float> img28;
        resample_bilinear_28_from_bgra(src, srcW, srcH, sx0, sy0, side, img28);

        // 4) 가우시안 블러(노이즈 억제)
        gaussian_blur_5x5(img28);

        // 5) Otsu 임계화 (바이너리 마스크 생성)
        const float th = otsu_threshold_0_1(img28);
        std::vector<float> mask28(img28.size(), 0.f);
        for (size_t i = 0; i < img28.size(); ++i)
            mask28[i] = (img28[i] >= th) ? 1.f : 0.f;

        // 6) 중심 모멘트 기반 중앙화 (질량중심을 (13.5,13.5)로 이동)
        auto [cx, cy] = centroid(mask28); // 바이너리 기준
        const double target = 13.5;       // 0..27 중심
        const double dx = cx - target;
        const double dy = cy - target;

        std::vector<float> centered28;
        translate_bilinear_28(img28, dx, dy, centered28);

        // 7) z-score 표준화 (MNIST/PyTorch 일반값)
        //    주의: 모델이 0..1 입력을 기대한다면 이 단계를 제거/비활성화해야 할 수 있습니다.
        constexpr float MEAN = 0.1307f;
        constexpr float STD = 0.3081f;
        zscore_normalize(centered28, MEAN, STD);

        outTensor28x28 = std::move(centered28);
        co_return true;
    }

    // ----- ONNX Runtime 숫자 인식 -----
    winrt::fire_and_forget MainWindow::RecognizeButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        hstring err;
        try
        {
            co_await m_ui; // UI thread

            if (m_strokes.empty())
            {
                co_await ShowErrorDialog(L"먼저 캔버스에 숫자를 적어주세요.");
                co_return;
            }

            // 입력 텐서 준비
            std::vector<float> inputData;
            bool ok = co_await TryRenderToMnistTensor(inputData);
            if (!ok)
            {
                co_await ShowErrorDialog(L"화면 캡처/전처리 중 문제가 발생했습니다.");
                co_return;
            }

            // 모델 선택 (UI)
            Windows::Storage::Pickers::FileOpenPicker picker;
            InitializePickerWithWindow(picker);
            picker.FileTypeFilter().Append(L".onnx");
            picker.SuggestedStartLocation(Windows::Storage::Pickers::PickerLocationId::DocumentsLibrary);
            Windows::Storage::StorageFile modelFile = co_await picker.PickSingleFileAsync();
            if (!modelFile) co_return;

            // 이후는 연산(비 UI)
            std::wstring modelPath = modelFile.Path().c_str();

            Ort::Env& env = GetOrtEnv();
            Ort::SessionOptions so;
            so.SetIntraOpNumThreads(1);
            so.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

            Ort::Session session(env, modelPath.c_str(), so);

            Ort::AllocatorWithDefaultOptions allocator;
            Ort::AllocatedStringPtr inputName = session.GetInputNameAllocated(0, allocator);
            Ort::AllocatedStringPtr outputName = session.GetOutputNameAllocated(0, allocator);

            std::array<int64_t, 4> dims{ 1, 1, 28, 28 };
            Ort::MemoryInfo memInfo = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtDeviceAllocator, OrtMemTypeDefault);
            Ort::Value inputTensor = Ort::Value::CreateTensor<float>(memInfo,
                inputData.data(), inputData.size(), dims.data(), dims.size());

            const char* input_names[] = { inputName.get() };
            const char* output_names[] = { outputName.get() };

            auto output = session.Run(Ort::RunOptions{ nullptr },
                input_names, &inputTensor, 1,
                output_names, 1);

            if (output.size() != 1 || !output[0].IsTensor())
                throw hresult_error(E_FAIL, L"모델 출력 형식이 예상과 다릅니다.");

            float* outData = output[0].GetTensorMutableData<float>();
            size_t outCount = output[0].GetTensorTypeAndShapeInfo().GetElementCount();
            if (outCount < 10) throw hresult_error(E_FAIL, L"출력 차원(10) 확인 실패.");

            float maxLogit = outData[0];
            int pred = 0;
            for (size_t i = 1; i < outCount; ++i)
                if (outData[i] > maxLogit) { maxLogit = outData[i]; pred = static_cast<int>(i); }

            double sumExp = 0.0;
            for (size_t i = 0; i < outCount; ++i)
                sumExp += std::exp(static_cast<double>(outData[i] - maxLogit));
            double prob = std::exp(static_cast<double>(outData[pred] - maxLogit)) / sumExp;

            co_await m_ui; // UI dialog
            ContentDialog dlg{};
            dlg.XamlRoot(this->Content().XamlRoot());
            dlg.Title(box_value(L"숫자 인식 결과"));
            std::wstringstream ss;
            ss << L"예측 숫자: " << pred << L"\n신뢰도: " << static_cast<int>(std::round(prob * 100.0)) << L"%";
            dlg.Content(box_value(hstring(ss.str())));
            dlg.CloseButtonText(L"확인");
            co_await dlg.ShowAsync();
        }
        catch (winrt::hresult_error const& ex)
        {
            hstring msg = ex.message();
            err = L"인식 중 예외가 발생했습니다.\n\n" + msg;
        }
        catch (std::exception const& ex)
        {
            err = L"인식 중 std::exception: " + to_hstring(ex.what());
        }
        catch (...)
        {
            err = L"인식 중 알 수 없는 오류가 발생했습니다.";
        }

        if (!err.empty()) co_await ShowErrorDialog(err);
        co_return;
    }
}
