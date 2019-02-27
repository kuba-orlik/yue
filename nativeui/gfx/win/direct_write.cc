// Copyright 2019 Cheng Zhao. All rights reserved.
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#include "nativeui/gfx/win/direct_write.h"

#include <shlwapi.h>
#include <wrl/client.h>

#include "base/debug/alias.h"
#include "nativeui/app.h"
#include "nativeui/gfx/font.h"
#include "nativeui/gfx/geometry/rect.h"
#include "nativeui/state.h"

namespace nu {

namespace {

class TextRenderer : public IDWriteTextRenderer {
 public:
  TextRenderer(IDWriteBitmapRenderTarget* target,
               IDWriteRenderingParams* rendering_params,
               float scale_factor)
      : target_(target),
        rendering_params_(rendering_params),
        scale_factor_(scale_factor) {}

  ~TextRenderer() {}

  // IDWriteTextRenderer:
  HRESULT __stdcall DrawGlyphRun(
      void* clientDrawingContext,
      FLOAT baselineOriginX,
      FLOAT baselineOriginY,
      DWRITE_MEASURING_MODE measuringMode,
      const DWRITE_GLYPH_RUN* glyphRun,
      const DWRITE_GLYPH_RUN_DESCRIPTION* glyphRunDescription,
      IUnknown* clientDrawingEffect) override {
    RECT dirty = {0};
    return target_->DrawGlyphRun(baselineOriginX,
                                 baselineOriginY,
                                 measuringMode,
                                 glyphRun,
                                 rendering_params_,
                                 RGB(0, 0, 0),
                                 &dirty);
  }

  HRESULT __stdcall DrawUnderline(
      void* clientDrawingContext,
      FLOAT baselineOriginX,
      FLOAT baselineOriginY,
      const DWRITE_UNDERLINE* underline,
      IUnknown* clientDrawingEffect) override {
    return E_NOTIMPL;
  }

  HRESULT __stdcall DrawStrikethrough(
      void* clientDrawingContext,
      FLOAT baselineOriginX,
      FLOAT baselineOriginY,
      const DWRITE_STRIKETHROUGH* strikethrough,
      IUnknown* clientDrawingEffect) override {
    return E_NOTIMPL;
  }

  HRESULT __stdcall DrawInlineObject(
      void* clientDrawingContext,
      FLOAT originX,
      FLOAT originY,
      IDWriteInlineObject* inlineObject,
      BOOL isSideways,
      BOOL isRightToLeft,
      IUnknown* clientDrawingEffect) override {
    return E_NOTIMPL;
  }

  // IDWritePixelSnapping:
  HRESULT __stdcall IsPixelSnappingDisabled(
      void* clientDrawingContext,
      BOOL* isDisabled) override {
    *isDisabled = FALSE;
    return S_OK;
  }

  HRESULT __stdcall GetCurrentTransform(
      void* clientDrawingContext,
      DWRITE_MATRIX* transform) override {
    return target_->GetCurrentTransform(transform);
  }

  HRESULT __stdcall GetPixelsPerDip(
      void* clientDrawingContext,
      FLOAT* pixelsPerDip) override {
    *pixelsPerDip = target_->GetPixelsPerDip();
    return S_OK;
  }

  // IUnknown:
  HRESULT __stdcall QueryInterface(const IID& riid, void** ppvObject) override {
    const QITAB QITable[] = {
      QITABENT(TextRenderer, IDWriteTextRenderer),
      QITABENT(TextRenderer, IDWritePixelSnapping),
      { 0 },
    };
    return QISearch(this, QITable, riid, ppvObject);
  }

  ULONG __stdcall AddRef() override {
    return InterlockedIncrement(&ref_);
  }

  ULONG __stdcall Release() override {
    auto cref = InterlockedDecrement(&ref_);
    if (cref == 0)
      delete this;
    return cref;
  }

 private:
  IDWriteBitmapRenderTarget* target_;  // weak ref
  IDWriteRenderingParams* rendering_params_;  // weak ref
  float scale_factor_;

  LONG ref_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TextRenderer);
};

}  // namespace

void CreateDWriteFactory(IDWriteFactory** factory) {
  Microsoft::WRL::ComPtr<IUnknown> factory_unknown;
  HRESULT hr =
      DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                          factory_unknown.GetAddressOf());
  if (FAILED(hr)) {
    base::debug::Alias(&hr);
    CHECK(false);
    return;
  }
  factory_unknown.CopyTo(factory);
}

bool CreateTextLayout(const base::string16& text,
                      IDWriteTextLayout** text_layout) {
  IDWriteFactory* factory = State::GetCurrent()->GetDWriteFactory();

  scoped_refptr<Font> default_font = App::GetCurrent()->GetDefaultFont();
  Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
  if (FAILED(factory->CreateTextFormat(default_font->GetName16().c_str(),
                                       nullptr,
                                       DWRITE_FONT_WEIGHT_NORMAL,
                                       DWRITE_FONT_STYLE_NORMAL,
                                       DWRITE_FONT_STRETCH_NORMAL,
                                       default_font->GetSize(),
                                       L"",
                                       format.GetAddressOf()))) {
    return false;
  }

  return SUCCEEDED(factory->CreateTextLayout(text.c_str(),
                                             static_cast<UINT32>(text.size()),
                                             format.Get(), FLT_MAX, FLT_MAX,
                                             text_layout));
}

bool WriteTextLayoutToHDC(HDC hdc,
                          const nu::Rect& rect,
                          float scale_factor,
                          IDWriteTextLayout* text_layout) {
  IDWriteFactory* factory = State::GetCurrent()->GetDWriteFactory();

  Microsoft::WRL::ComPtr<IDWriteGdiInterop> interop;
  if (FAILED(factory->GetGdiInterop(interop.GetAddressOf())))
    return false;

  Microsoft::WRL::ComPtr<IDWriteBitmapRenderTarget> target;
  if (FAILED(interop->CreateBitmapRenderTarget(hdc,
                                               rect.width(), rect.height(),
                                               target.GetAddressOf())))
    return false;

  if (FAILED(target->SetPixelsPerDip(scale_factor)))
    return false;

  Microsoft::WRL::ComPtr<IDWriteRenderingParams> rendering_params;
  if (FAILED(factory->CreateRenderingParams(rendering_params.GetAddressOf())))
    return false;

  HDC memdc = target->GetMemoryDC();
  BitBlt(memdc, 0, 0, rect.width(), rect.height(),
         hdc, rect.x(), rect.y(), SRCCOPY);

  text_layout->SetMaxWidth(std::ceil(rect.width() / scale_factor));
  text_layout->SetMaxHeight(std::ceil(rect.height() / scale_factor));

  scoped_refptr<TextRenderer> renderer =
      new TextRenderer(target.Get(), rendering_params.Get(), scale_factor);
  if (FAILED(text_layout->Draw(nullptr, renderer.get(), 0, 0)))
    return false;

  BitBlt(hdc, 0, 0, rect.width(), rect.height(),
         memdc, rect.x(), rect.y(), SRCCOPY | NOMIRRORBITMAP);
  return true;
}

}  // namespace nu
