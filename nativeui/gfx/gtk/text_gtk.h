// Copyright 2019 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#ifndef NATIVEUI_GFX_GTK_TEXT_GTK_H_
#define NATIVEUI_GFX_GTK_TEXT_GTK_H_

#include <pango/pango.h>

namespace nu {

class SizeF;
struct TextDrawOptions;

void SetupPangoLayout(PangoLayout* layout,
                      const SizeF& size,
                      const TextDrawOptions& options);

}  // namespace nu

#endif  // NATIVEUI_GFX_GTK_TEXT_GTK_H_
