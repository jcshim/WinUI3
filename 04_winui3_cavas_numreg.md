# 0) 준비

* NuGet 추가

  * **Win2D.WinUI** (WinUI 3용 Win2D)
  * **Microsoft.ML.OnnxRuntime** (CPU)
    *(GPU 원하면 `Microsoft.ML.OnnxRuntime.DirectML`로 교체 가능)*
* ONNX 모델: MNIST 호환(입력 `[1,1,28,28]`)

---

# 1) XAML (MainWindow.xaml)

```xml
<Window
    x:Class="winrt::MyApp::implementation::MainWindow"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:win2d="using:Microsoft.Graphics.Canvas.UI.Xaml">
  <Grid>
    <win2d:CanvasControl x:Name="Canvas"
                         Draw="OnDraw"
                         PointerPressed="OnDown"
                         PointerMoved="OnMove"
                         PointerReleased="OnUp"/>
  </Grid>
</Window>
```

---

# 2) C++/WinRT 코드 (MainWindow.xaml.h / .cpp)

## 헤더

```cpp
// .h
#include <winrt/Microsoft.Graphics.Canvas.h>
#include <winrt/Microsoft.Graphics.Canvas.UI.Xaml.h>
#include <winrt/Windows.Foundation.Numerics.h>
#include <vector>

// ONNX Runtime
#include <onnxruntime_cxx_api.h>
```

## 필드

```cpp
// .cpp
using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Numerics;
using namespace Microsoft::Graphics::Canvas;
using namespace Microsoft::Graphics::Canvas::UI::Xaml;

std::vector<std::vector<float2>> g_strokes; // 폴리라인들의 집합
bool g_drawing = false;

// ONNX 관련(앱 시작 시 초기화 권장: 생성자에서)
Ort::Env g_env{ORT_LOGGING_LEVEL_WARNING, "mnist"};
std::unique_ptr<Ort::Session> g_sess;
std::vector<const char*> g_inputNames, g_outputNames;
```

## ONNX 세션 초기화(예: MainWindow 생성자)

```cpp
MainWindow::MainWindow()
{
    InitializeComponent();

    Ort::SessionOptions so;
    // CPU: 기본. GPU(DirectML) 쓰려면 EP 추가 코드 필요.
    g_sess = std::make_unique<Ort::Session>(g_env, L"Assets\\mnist-8.onnx", so);

    // 이름 캐시(Optional)
    size_t nInput = g_sess->GetInputCount();
    size_t nOutput = g_sess->GetOutputCount();
    Ort::AllocatorWithDefaultOptions alloc;

    for (size_t i=0;i<nInput;i++) {
        auto name = g_sess->GetInputNameAllocated(i, alloc);
        g_inputNames.push_back(strdup(name.get()));
    }
    for (size_t i=0;i<nOutput;i++) {
        auto name = g_sess->GetOutputNameAllocated(i, alloc);
        g_outputNames.push_back(strdup(name.get()));
    }
}
```

## 포인터 입력 수집

```cpp
void MainWindow::OnDown(IInspectable const&, Input::PointerRoutedEventArgs const& e)
{
    g_drawing = true;
    g_strokes.emplace_back();
    auto pos = e.GetCurrentPoint(Canvas()).Position();
    g_strokes.back().push_back(float2{ (float)pos.X, (float)pos.Y });
    Canvas().Invalidate();
}

void MainWindow::OnMove(IInspectable const&, Input::PointerRoutedEventArgs const& e)
{
    if (!g_drawing) return;
    auto pos = e.GetCurrentPoint(Canvas()).Position();
    g_strokes.back().push_back(float2{ (float)pos.X, (float)pos.Y });
    Canvas().Invalidate();
}

void MainWindow::OnUp(IInspectable const&, Input::PointerRoutedEventArgs const&)
{
    g_drawing = false;

    // 여기서 인식 파이프라인 트리거 (Released 시 1회 실행)
    RecognizeDigit();
}
```

## 그리기(Win2D)

```cpp
void MainWindow::OnDraw(CanvasControl const& sender, CanvasDrawEventArgs const& args)
{
    auto ds = args.DrawingSession();
    ds.Clear(Windows::UI::Colors::Black());

    // 흰 선으로 폴리라인 렌더(펜 굵기: 12)
    for (auto& poly : g_strokes) {
        for (size_t i=1; i<poly.size(); ++i) {
            ds.DrawLine(poly[i-1], poly[i], Windows::UI::Colors::White(), 12.0f);
        }
    }
}
```

---

# 3) 래스터화(픽셀 뽑기 → 전처리 28×28)

```cpp
std::vector<uint8_t> CaptureToGrayscale28x28(CanvasControl const& cc)
{
    // 1) 오프스크린 렌더
    auto sz = cc.Size();
    CanvasDevice dev = CanvasDevice::GetSharedDevice();
    CanvasRenderTarget rt(dev, (float)sz.Width, (float)sz.Height, 96.0f);

    {
        auto ds = rt.CreateDrawingSession();
        ds.Clear(Windows::UI::Colors::Black());
        for (auto& poly : g_strokes)
            for (size_t i=1;i<poly.size();++i)
                ds.DrawLine(poly[i-1], poly[i], Windows::UI::Colors::White(), 12.0f);
    }

    // 2) BGRA8 읽기
    std::vector<uint8_t> bgra(rt.SizeInPixels().Width * rt.SizeInPixels().Height * 4);
    rt.GetPixelBytes(bgra);

    // 3) 그레이 + 이진화 + bbox 추출
    const int W = rt.SizeInPixels().Width;
    const int H = rt.SizeInPixels().Height;
    std::vector<uint8_t> gray(W*H);

    auto toGray = [&](int i)->uint8_t {
        uint8_t b=bgra[4*i+0], g=bgra[4*i+1], r=bgra[4*i+2];
        int v = (int)(0.299*r + 0.587*g + 0.114*b);
        return (uint8_t)std::clamp(v,0,255);
    };
    for (int i=0;i<W*H;i++) gray[i] = toGray(i);

    // 흰 획(높은 값) 기준 간단 임계값
    const int TH = 128;
    int minx=W, miny=H, maxx=-1, maxy=-1;
    for (int y=0;y<H;y++){
        for (int x=0;x<W;x++){
            auto v = gray[y*W+x] > TH ? 255 : 0;
            gray[y*W+x] = (uint8_t)v;
            if (v){
                if (x<minx) minx=x; if (x>maxx) maxx=x;
                if (y<miny) miny=y; if (y>maxy) maxy=y;
            }
        }
    }
    if (maxx<minx || maxy<miny) {
        // 빈 이미지면 28x28 0 채움
        return std::vector<uint8_t>(28*28, 0);
    }

    // 4) bbox 크롭 → 긴 변 20으로 스케일 → 28x28 중앙 배치
    int bw = maxx-minx+1, bh = maxy-miny+1;
    float scale = 20.0f / (float)std::max(bw, bh);
    int nw = std::max(1, (int)std::round(bw*scale));
    int nh = std::max(1, (int)std::round(bh*scale));

    // 간단 최근접 보간
    std::vector<uint8_t> scaled(nw*nh);
    for (int j=0;j<nh;j++){
        for (int i=0;i<nw;i++){
            int sx = minx + std::min(bw-1, (int)std::round(i/scale));
            int sy = miny + std::min(bh-1, (int)std::round(j/scale));
            scaled[j*nw+i] = gray[sy*W+sx];
        }
    }

    // 28x28 블랙 캔버스에 중앙 배치
    std::vector<uint8_t> img28(28*28, 0);
    int ox = (28 - nw)/2;
    int oy = (28 - nh)/2;
    for (int j=0;j<nh;j++)
        for (int i=0;i<nw;i++)
            img28[(oy+j)*28 + (ox+i)] = scaled[j*nw+i];

    // (고급 팁) 질량중심(COM) 정렬을 추가하면 MNIST와 더 유사해집니다.
    return img28;
}
```

---

# 4) ONNX 추론(CPU)

```cpp
int RunOnnxDigit(const std::vector<uint8_t>& img28x28)
{
    // 입력: float[1,1,28,28] (0~1 정규화)
    std::vector<float> input(1*1*28*28);
    for (int i=0;i<28*28;i++) input[i] = img28x28[i] / 255.0f;

    Ort::AllocatorWithDefaultOptions alloc;
    auto memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    std::array<int64_t,4> shape{1,1,28,28};
    Ort::Value inTensor = Ort::Value::CreateTensor<float>(memInfo, input.data(), input.size(), shape.data(), shape.size());

    auto output = g_sess->Run(Ort::RunOptions{nullptr},
                              g_inputNames.data(), &inTensor, 1,
                              g_outputNames.data(), (size_t)g_outputNames.size());

    // 출력: [1,10]
    float* p = output.front().GetTensorMutableData<float>();
    int best = 0;
    for (int c=1;c<10;c++) if (p[c] > p[best]) best = c;
    return best;
}
```

---

# 5) 파이프라인 연결(Released에서 호출)

```cpp
void MainWindow::RecognizeDigit()
{
    auto img28 = CaptureToGrayscale28x28(Canvas());
    int digit = RunOnnxDigit(img28);

    // 결과 표시: 타이틀/오버레이 등
    Title(box_value(L"예측: " + std::to_wstring(digit)));
}
```

---

## 정확도/안정성 팁

* **극성(MNIST 기준)**: 배경=검정(0), 획=흰색(255). 이미 위 코드가 그 방향입니다.
* **스케일 & 중앙 정렬**: 긴 변 20 스케일 → 28 중앙 배치. (추가로 COM 정렬까지 구현 시 성능 ↑)
* **추론 타이밍**: 포인터 **Released 시 1회**만 추론(성능).
* **모델 교체**: 어린이 필체에 맞춘 **재학습/파인튜닝** 모델을 쓰면 효과가 큽니다.

---

## 왜 이 구성이 초보자에게 적합한가?

* **Windows Ink 없이도**: Win2D `CanvasControl`의 포인터 이벤트로 손쉽게 스트로크 수집/렌더.
* **의존성 단순**: Win2D + ONNX Runtime 2개 NuGet이면 끝.
* **확장 용이**: 가이드라인/채점 히트맵, 스트로크 스무딩(카르만/사보레츠키-골레이) 등을 `Draw`에서 바로 오버레이 가능.
