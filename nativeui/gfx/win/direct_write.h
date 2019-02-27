// Copyright 2019 Cheng Zhao. All rights reserved.
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef NATIVEUI_GFX_WIN_DIRECT_WRITE_H_
#define NATIVEUI_GFX_WIN_DIRECT_WRITE_H_

#include <dwrite.h>

#include "base/strings/string16.h"

namespace nu {

class Rect;

// Create a DirectWrite factory.
void CreateDWriteFactory(IDWriteFactory** factory);

// Create a text layout with default font.
bool CreateTextLayout(const base::string16& text,
                      IDWriteTextLayout** text_layout);

// Write text layout to HDC.
bool WriteTextLayoutToHDC(HDC hdc,
                          const nu::Rect& rect,
                          float scale_factor,
                          IDWriteTextLayout* text_layout);

}  // namespace nu

#endif  // NATIVEUI_GFX_WIN_DIRECT_WRITE_H_
