// Copyright 2016 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#ifndef NATIVEUI_GFX_WIN_PAINTER_WIN_H_
#define NATIVEUI_GFX_WIN_PAINTER_WIN_H_

#include <wrl/client.h>

#include <stack>
#include <string>

#include "nativeui/gfx/painter.h"
#include "nativeui/gfx/win/gdiplus.h"
#include "nativeui/gfx/win/native_theme.h"

namespace nu {

class DWriteTextRenderer;

class PainterWin : public Painter {
 public:
  // Paint on existing target.
  PainterWin(ID2D1RenderTarget* target, HDC hdc);

  // Paint on offscreen buffer.
  PainterWin(DoubleBuffer* buffer, float scale_factor);

  // PainterWin should be created on stack for best performance.
  ~PainterWin() override;

  // Call EndDraw and return Whether we should recreate target.
  bool EndDraw();

  // Draw a control.
  void DrawNativeTheme(NativeTheme::Part part,
                       ControlState state,
                       const RectF& rect,
                       const RectF& dirty,
                       const NativeTheme::ExtraParams& extra);

  // Draw the focus rect.
  void DrawFocusRect(const RectF& rect);

  // Painter:
  void Save() override;
  void Restore() override;
  void BeginPath() override;
  void ClosePath() override;
  void MoveTo(const PointF& point) override;
  void LineTo(const PointF& point) override;
  void BezierCurveTo(const PointF& cp1,
                     const PointF& cp2,
                     const PointF& ep) override;
  void Arc(const PointF& point, float radius, float sa, float ea) override;
  void Rect(const RectF& rect) override;
  void Clip() override;
  void ClipRect(const RectF& rect) override;
  void Translate(const Vector2dF& offset) override;
  void Rotate(float angle) override;
  void Scale(const Vector2dF& scale) override;
  void SetColor(Color color) override;
  void SetStrokeColor(Color color) override;
  void SetFillColor(Color color) override;
  void SetLineWidth(float width) override;
  void Stroke() override;
  void Fill() override;
  void StrokeRect(const RectF& rect) override;
  void FillRect(const RectF& rect) override;
  void DrawImage(Image* image, const RectF& rect) override;
  void DrawImageFromRect(Image* image, const RectF& src,
                         const RectF& dest) override;
  void DrawCanvas(Canvas* canvas, const RectF& rect) override;
  void DrawCanvasFromRect(Canvas* canvas, const RectF& src,
                          const RectF& dest) override;
  void DrawAttributedText(AttributedText* text, const RectF& rect) override;

 private:
  // Popup current layer.
  void PopLayer();

  // The saved state.
  struct PainterState {
    float line_width = 1.f;
    Color stroke_color;
    Color fill_color;
    D2D1::Matrix3x2F matrix = D2D1::Matrix3x2F::Identity();
    Microsoft::WRL::ComPtr<ID2D1DrawingStateBlock> state;
    Microsoft::WRL::ComPtr<ID2D1PathGeometry> clip;
    Microsoft::WRL::ComPtr<ID2D1Layer> layer;
    bool layer_changed = false;
  };

  // Return the top state.
  PainterState& top() { return states_.top(); }
  float& line_width() { return top().line_width; }
  Color& stroke_color() { return top().stroke_color; }
  Color& fill_color() { return top().fill_color; }
  D2D1::Matrix3x2F& matrix() { return top().matrix; }

  // The stack for all saved states.
  std::stack<PainterState> states_;

  ID2D1Factory* factory_;
  ID2D1RenderTarget* target_;
  HDC hdc_;
  float scale_factor_;

  scoped_refptr<DWriteTextRenderer> text_renderer_;

  // Current path.
  Microsoft::WRL::ComPtr<ID2D1PathGeometry> path_;
  Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink_;

  // Whether a figure is in-progress.
  bool in_figure_ = false;

  // Points information of current path.
  PointF start_point_;
  PointF last_point_;
};

}  // namespace nu

#endif  // NATIVEUI_GFX_WIN_PAINTER_WIN_H_
