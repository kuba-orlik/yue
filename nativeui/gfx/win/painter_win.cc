// Copyright 2016 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#include "nativeui/gfx/win/painter_win.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <wincodec.h>

#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "nativeui/gfx/attributed_text.h"
#include "nativeui/gfx/canvas.h"
#include "nativeui/gfx/font.h"
#include "nativeui/gfx/geometry/rect_conversions.h"
#include "nativeui/gfx/geometry/size_conversions.h"
#include "nativeui/gfx/image.h"
#include "nativeui/gfx/win/direct_write.h"
#include "nativeui/gfx/win/double_buffer.h"
#include "nativeui/gfx/win/dwrite_text_renderer.h"
#include "nativeui/gfx/win/screen_win.h"
#include "nativeui/state.h"
#include "nativeui/system.h"

namespace nu {

namespace {

ID2D1RenderTarget* CreateDCRenderTarget(
    HDC hdc, const Size& size, float scale_factor) {
  float dpi = GetDPIFromScalingFactor(scale_factor);

  Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> target;
  D2D1_RENDER_TARGET_PROPERTIES properties = {
    D2D1_RENDER_TARGET_TYPE_DEFAULT,
    {DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED},
    dpi, dpi,
    D2D1_RENDER_TARGET_USAGE_NONE,
    D2D1_FEATURE_LEVEL_DEFAULT,
  };
  CHECK(SUCCEEDED(State::GetCurrent()->GetD2D1Factory()->CreateDCRenderTarget(
                      &properties, &target)));

  RECT rc = nu::Rect(size).ToRECT();
  target->BindDC(hdc, &rc);
  target->SetDpi(dpi, dpi);
  return target.Detach();
}

// Code modified from Microsoft/WinObjC:
// WinObjC/Frameworks/CoreGraphics/CGPath.mm
//
// Copyright (c) 2016 Microsoft Corporation. All rights reserved.
// This code is licensed under the MIT License (MIT).
//
// This function will return a normalized angle in radians between 0 and 2pi.
// This is to standardize the calculations for arcs since 0, 2pi, 4pi, etc...
// are all visually the same angle.
inline float NormalizeAngle(float angle) {
  float normalized = fmod(angle, 2.f * M_PI);
  if (abs(normalized) < .00001 /* zeroAngleThreshold */)
    normalized = 0;
  if (normalized == 0 && angle != 0)
    return static_cast<float>(2.f * M_PI);
  return normalized;
}

inline PointF PointOnAngle(float angle, float radius, int x, int y) {
  return PointF(x + radius * cos(angle), y + radius * sin(angle));
}

}  // namespace

PainterWin::PainterWin(ID2D1RenderTarget* target, HDC hdc)
    : factory_(State::GetCurrent()->GetD2D1Factory()),
      target_(target),
      hdc_(hdc) {
  // Initial state.
  states_.emplace(PainterState());

  // Read scale factor.
  float dpi;
  target_->GetDpi(&dpi, &dpi);
  scale_factor_ = GetScalingFactorFromDPI(dpi);

  target_->SetTransform(matrix());
  target_->BeginDraw();
}

PainterWin::PainterWin(HDC hdc, const Size& size, float scale_factor)
    : PainterWin(CreateDCRenderTarget(hdc, size, scale_factor), hdc) {
  should_release_ = true;
}

PainterWin::~PainterWin() {
  EndDraw();
}

bool PainterWin::EndDraw() {
  if (!target_)
    return false;

  PopLayer();
  bool recreate = target_->EndDraw() == D2DERR_RECREATE_TARGET;

  if (should_release_)
    target_->Release();

  target_ = nullptr;
  return recreate;
}

void PainterWin::DrawNativeTheme(NativeTheme::Part part,
                                 ControlState state,
                                 const RectF& rect,
                                 const RectF& dirty,
                                 const NativeTheme::ExtraParams& extra) {
  // Is there nothing to draw.
  if (rect.size().IsEmpty())
    return;
  RectF intersect(rect);
  intersect.Intersect(dirty);
  if (intersect.IsEmpty())
    return;

  // Only draw the part that needs to be refreshed.
  RectF src_rect = rect - intersect.OffsetFromOrigin();
  nu::Rect src = ToEnclosingRect(ScaleRect(src_rect, scale_factor_));
  Size size = ToCeiledSize(ScaleSize(intersect.size(), scale_factor_));

  // Draw the part on offscreen buffer.
  DoubleBuffer buffer(hdc_, size);
  State::GetCurrent()->GetNativeTheme()->Paint(
      part, buffer.dc(), state, src, extra);

  // Convert offscreen buffer to bitmap.
  IWICImagingFactory* wic_factory = State::GetCurrent()->GetWICFactory();
  Microsoft::WRL::ComPtr<IWICBitmap> wic_bitmap;
  wic_factory->CreateBitmapFromHBITMAP(
      buffer.bitmap(), NULL, WICBitmapUsePremultipliedAlpha, &wic_bitmap);
  // FIXME(zcbenz): This is extremely slow when the component has a large size,
  // we should cache the result in views.
  Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;
  target_->CreateBitmapFromWicBitmap(wic_bitmap.Get(), &bitmap);

  // Copy the dirty part back.
  target_->DrawBitmap(bitmap.Get(), intersect.ToD2D1());
}

void PainterWin::DrawFocusRect(const nu::RectF& rect) {
  static Color ring_color = System::GetColor(System::Color::DisabledText);
  Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
  target_->CreateSolidColorBrush(ring_color.ToD2D1(), &brush);
  Microsoft::WRL::ComPtr<ID2D1StrokeStyle> style;
  float dashes[] = {2.f, 2.f};
  factory_->CreateStrokeStyle(
      D2D1::StrokeStyleProperties(
          D2D1_CAP_STYLE_FLAT,
          D2D1_CAP_STYLE_FLAT,
          D2D1_CAP_STYLE_FLAT,
          D2D1_LINE_JOIN_MITER,
          10.0f,
          D2D1_DASH_STYLE_CUSTOM,
          0.0f),
      dashes, arraysize(dashes), &style);
  target_->DrawRectangle(rect.ToD2D1(), brush.Get(), 1.f, style.Get());
}

void PainterWin::Save() {
  states_.push(top());
  top().layer_changed = false;
  if (SUCCEEDED(factory_->CreateDrawingStateBlock(nullptr, nullptr,
                                                  &top().state)))
    target_->SaveDrawingState(top().state.Get());
  else
    LOG(ERROR) << "Failed to create drawing state block";
}

void PainterWin::Restore() {
  if (states_.size() == 1)
    return;
  target_->RestoreDrawingState(top().state.Get());
  PopLayer();
  states_.pop();
}

void PainterWin::BeginPath() {
  factory_->CreatePathGeometry(&path_);
  path_->Open(&sink_);

  in_figure_ = false;
  start_point_ = last_point_ = PointF();
}

void PainterWin::ClosePath() {
  if (sink_) {
    if (in_figure_) {
      sink_->EndFigure(D2D1_FIGURE_END_CLOSED);
    }
    sink_->Close();
    sink_.Reset();
  }

  in_figure_ = false;
  start_point_ = last_point_ = PointF();
}

void PainterWin::MoveTo(const PointF& point) {
  if (!sink_)
    BeginPath();
  if (in_figure_)
    sink_->EndFigure(D2D1_FIGURE_END_OPEN);
  sink_->BeginFigure(point.ToD2D1(), D2D1_FIGURE_BEGIN_FILLED);

  in_figure_ = true;
  start_point_ = last_point_ = point;
}

void PainterWin::LineTo(const PointF& point) {
  if (!sink_)
    MoveTo(last_point_);
  sink_->AddLine(point.ToD2D1());
  last_point_ = point;
}

void PainterWin::BezierCurveTo(const PointF& cp1,
                               const PointF& cp2,
                               const PointF& ep) {
  if (!sink_)
    MoveTo(last_point_);
  sink_->AddBezier({cp1.ToD2D1(), cp2.ToD2D1(), ep.ToD2D1()});
  last_point_ = ep;
}

void PainterWin::Arc(const PointF& point, float radius, float sa, float ea) {
  // Code modified from Microsoft/WinObjC:
  // WinObjC/Frameworks/CoreGraphics/CGPath.mm
  //
  // Copyright (c) 2016 Microsoft Corporation. All rights reserved.
  // This code is licensed under the MIT License (MIT).

  // Normalize the angles to work with to values between 0 and 2*PI
  sa = NormalizeAngle(sa);
  ea = NormalizeAngle(ea);

  PointF startPoint = PointOnAngle(sa, radius, point.x(), point.y());
  PointF endPoint = PointOnAngle(ea, radius, point.x(), point.y());

  // Create the parameters for the AddArc method.
  D2D1_SIZE_F radiusD2D = {radius, radius};
  D2D1_ARC_SIZE arcSize = D2D1_ARC_SIZE_SMALL;

  // D2D does not understand that the ending angle must be pointing in the
  // proper direction, thus we must translate what it means to have an ending
  // angle to the proper small arc or large arc that D2D will use since a circle
  // will intersect that point regardless of which direction it is drawn in.
  const bool clockwise = false;
  float rawDifference = 0;
  if (clockwise)
    rawDifference = sa - ea;
  else
    rawDifference = ea - sa;

  float difference = rawDifference;
  if (difference < 0)
    difference += static_cast<float>(2 * M_PI);
  if (difference > M_PI)
    arcSize = D2D1_ARC_SIZE_LARGE;

  rawDifference = abs(rawDifference);
  if (!clockwise)
    rawDifference *= -1;

  // The direction of the arc's sweep must be reversed since the coordinate
  // systems for D2D and CoreGraphics are reversed. CW in D2D is CCW in
  // CoreGraphics.
  D2D1_SWEEP_DIRECTION sweepDirection = {
      clockwise ? D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE
                : D2D1_SWEEP_DIRECTION_CLOCKWISE
  };

  if (!sink_)
    BeginPath();
  if (in_figure_) {
    if (last_point_ != start_point_)
      sink_->AddLine(startPoint.ToD2D1());
  } else {
    sink_->BeginFigure(startPoint.ToD2D1(), D2D1_FIGURE_BEGIN_FILLED);
    start_point_ = last_point_ = startPoint;
  }

  // This will only happen when drawing a circle in the clockwise direction
  // from 2pi to 0, a scenario supported on the reference platform.
  if (abs(abs(rawDifference) - 2 * M_PI) < .00001 /* zeroAngleThreshold */) {
    float midPointAngle = sa + rawDifference / 2;
    PointF midPoint = PointOnAngle(midPointAngle, radius, point.x(), point.y());
    D2D1_ARC_SEGMENT arcSegment1 = D2D1::ArcSegment(
        midPoint.ToD2D1(), radiusD2D, rawDifference / 2, sweepDirection,
        D2D1_ARC_SIZE_SMALL);
    D2D1_ARC_SEGMENT arcSegment2 = D2D1::ArcSegment(
        endPoint.ToD2D1(), radiusD2D, rawDifference / 2, sweepDirection,
        D2D1_ARC_SIZE_SMALL);
    sink_->AddArc(arcSegment1);
    sink_->AddArc(arcSegment2);
  } else {
    D2D1_ARC_SEGMENT arcSegment = D2D1::ArcSegment(
        endPoint.ToD2D1(), radiusD2D, rawDifference, sweepDirection, arcSize);
    sink_->AddArc(arcSegment);
  }

  in_figure_ = true;
  last_point_ = point;
}

void PainterWin::Rect(const RectF& rect) {
  MoveTo(rect.origin());
  D2D1_POINT_2F lines[] = {
    {rect.right(), rect.y()},
    {rect.right(), rect.bottom()},
    {rect.x(), rect.bottom()},
    {rect.x(), rect.y()},
  };
  sink_->AddLines(lines, arraysize(lines));
  last_point_ = rect.origin();
}

void PainterWin::Clip() {
  if (!path_)
    return;

  ClosePath();
  PopLayer();

  // Create the layers only when it is changed.
  if (!top().layer_changed) {
    if (!top().clip) {
      // If there was no clip regions, simply reuse path.
      top().clip = path_;
    } else {
      Microsoft::WRL::ComPtr<ID2D1PathGeometry> clip;
      factory_->CreatePathGeometry(&clip);
      Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
      clip->Open(&sink);
      top().clip->CombineWithGeometry(path_.Get(), D2D1_COMBINE_MODE_INTERSECT,
                                      nullptr, sink.Get());
      sink->Close();
      top().clip = clip;
    }
    target_->CreateLayer(&top().layer);
    top().layer_changed = true;
  }

  // Apply new layer.
  target_->PushLayer(
      D2D1::LayerParameters(
          D2D1::InfiniteRect(),
          top().clip.Get(),
          target_->GetAntialiasMode(),
          D2D1::IdentityMatrix(),
          1.0,
          nullptr,
          D2D1_LAYER_OPTIONS_NONE),
      top().layer.Get());
}

void PainterWin::ClipRect(const RectF& rect) {
  BeginPath();
  Rect(rect);
  Clip();
}

void PainterWin::Translate(const Vector2dF& offset) {
  matrix() = matrix() * D2D1::Matrix3x2F::Translation(offset.x(), offset.y());
  target_->SetTransform(matrix());
}

void PainterWin::Rotate(float angle) {
  matrix() = D2D1::Matrix3x2F::Rotation(angle / M_PI * 180.f) * matrix();
  target_->SetTransform(matrix());
}

void PainterWin::Scale(const Vector2dF& scale) {
  matrix() = D2D1::Matrix3x2F::Scale(scale.x(), scale.y()) * matrix();
  target_->SetTransform(matrix());
}

void PainterWin::SetColor(Color color) {
  SetStrokeColor(color);
  SetFillColor(color);
}

void PainterWin::SetStrokeColor(Color color) {
  stroke_color() = color;
}

void PainterWin::SetFillColor(Color color) {
  fill_color() = color;
}

void PainterWin::SetLineWidth(float width) {
  line_width() = width;
}

void PainterWin::Stroke() {
  if (!path_)
    return;
  ClosePath();
  Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
  target_->CreateSolidColorBrush(stroke_color().ToD2D1(), &brush);
  target_->DrawGeometry(path_.Get(), brush.Get(), line_width());
}

void PainterWin::Fill() {
  if (!path_)
    return;
  ClosePath();
  Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
  target_->CreateSolidColorBrush(fill_color().ToD2D1(), &brush);
  target_->FillGeometry(path_.Get(), brush.Get());
}

void PainterWin::StrokeRect(const RectF& rect) {
  Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
  target_->CreateSolidColorBrush(stroke_color().ToD2D1(), &brush);
  target_->DrawRectangle(rect.ToD2D1(), brush.Get(), line_width(), nullptr);
}

void PainterWin::FillRect(const RectF& rect) {
  Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
  target_->CreateSolidColorBrush(fill_color().ToD2D1(), &brush);
  target_->FillRectangle(rect.ToD2D1(), brush.Get());
}

void PainterWin::DrawImage(Image* image, const RectF& rect) {
}

void PainterWin::DrawImageFromRect(Image* image, const RectF& src,
                                   const RectF& dest) {
}

void PainterWin::DrawCanvas(Canvas* canvas, const RectF& rect) {
}

void PainterWin::DrawCanvasFromRect(Canvas* canvas, const RectF& src,
                                    const RectF& dest) {
}

void PainterWin::DrawAttributedText(AttributedText* text, const RectF& rect) {
  if (!text_renderer_)
    text_renderer_ = new DWriteTextRenderer(target_);
  IDWriteTextLayout* text_layout = text->GetNative();
  text_layout->SetMaxWidth(rect.width());
  text_layout->SetMaxHeight(rect.height());
  text_layout->Draw(nullptr, text_renderer_.get(), rect.x(), rect.y());
}

void PainterWin::PopLayer() {
  if (top().layer && top().layer_changed) {
    target_->PopLayer();
    top().layer.Reset();
    top().layer_changed = false;
  }
}

}  // namespace nu
