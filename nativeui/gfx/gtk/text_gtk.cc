// Copyright 2019 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#include "nativeui/gfx/gtk/text_gtk.h"

#include "nativeui/gfx/text.h"

namespace nu {

void SetupPangoLayout(PangoLayout* layout,
                      const SizeF& size,
                      const TextDrawOptions& options) {
  pango_layout_set_ellipsize(layout,
                             options.ellipsis ? PANGO_ELLIPSIZE_END
                                              : PANGO_ELLIPSIZE_NONE);

  if (options.align == TextAlign::Start)
    pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
  else if (options.align == TextAlign::Center)
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
  else
    pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);

  if (options.wrap) {
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
    pango_layout_set_width(layout, size.width() * PANGO_SCALE);
    pango_layout_set_height(layout, size.height() * PANGO_SCALE);
  } else {
    pango_layout_set_width(layout, -1);
  }
}

}  // namespace nu
