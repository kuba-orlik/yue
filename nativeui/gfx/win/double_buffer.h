// Copyright 2016 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#ifndef NATIVEUI_GFX_WIN_DOUBLE_BUFFER_H_
#define NATIVEUI_GFX_WIN_DOUBLE_BUFFER_H_

#include <wrl/client.h>

#include <memory>

#include "base/win/scoped_gdi_object.h"
#include "base/win/scoped_hdc.h"
#include "base/win/scoped_select_object.h"
#include "nativeui/gfx/win/gdiplus.h"

typedef struct ID2D1RenderTarget ID2D1RenderTarget;

namespace nu {

// Create a memory buffer for |dc| and copy the result back in destructor.
class DoubleBuffer {
 public:
  DoubleBuffer(HWND hwnd, const Size& size);
  DoubleBuffer(HDC dc, const Size& size);
  ~DoubleBuffer();

  // Return a GDI+ bitmap with alpha channel.
  //
  // Note that the memory bitmap does not have alpha channel in it, to get a
  // transparent HBITMAP we should usually get the GDI+ bitmap and then create
  // HBITMAP with it.
  std::unique_ptr<Gdiplus::Bitmap> GetGdiplusBitmap() const;

  // Return a D2D1 bitmap.
  Microsoft::WRL::ComPtr<ID2D1Bitmap> GetD2D1Bitmap(
      ID2D1RenderTarget* target, float scale_factor) const;

  HDC dc() const { return mem_dc_.Get(); }
  HBITMAP bitmap() const { return mem_bitmap_.get(); }
  const Size& size() const { return size_; }

 private:
  HDC dc_;
  Size size_;
  base::win::ScopedCreateDC mem_dc_;
  base::win::ScopedBitmap mem_bitmap_;
  base::win::ScopedSelectObject select_bitmap_;
};

}  // namespace nu

#endif  // NATIVEUI_GFX_WIN_DOUBLE_BUFFER_H_
