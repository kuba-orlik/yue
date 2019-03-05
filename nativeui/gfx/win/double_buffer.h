// Copyright 2016 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#ifndef NATIVEUI_GFX_WIN_DOUBLE_BUFFER_H_
#define NATIVEUI_GFX_WIN_DOUBLE_BUFFER_H_

#include <memory>

#include "base/win/scoped_gdi_object.h"
#include "base/win/scoped_hdc.h"
#include "base/win/scoped_select_object.h"
#include "nativeui/gfx/win/gdiplus.h"

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

  HDC dc() const { return mem_dc_.Get(); }
  HBITMAP bitmap() const { return mem_bitmap_.get(); }

 private:
  HDC dc_;
  base::win::ScopedCreateDC mem_dc_;
  base::win::ScopedBitmap mem_bitmap_;
  base::win::ScopedSelectObject select_bitmap_;
};

}  // namespace nu

#endif  // NATIVEUI_GFX_WIN_DOUBLE_BUFFER_H_
