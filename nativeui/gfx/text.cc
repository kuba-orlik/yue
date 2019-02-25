// Copyright 2017 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#include "nativeui/gfx/text.h"

#include "nativeui/app.h"
#include "nativeui/gfx/font.h"

namespace nu {

TextAttributes::TextAttributes(Font* font, Color color, TextAlign align,
                               TextAlign valign, bool wrap, bool ellipsis)
    : TextDrawOptions({align, valign, wrap, ellipsis}),
      font(font), color(color) {}

TextAttributes::TextAttributes()
    : TextAttributes(App::GetCurrent()->GetDefaultFont(),
                     System::GetColor(System::Color::Text)) {}

TextAttributes::TextAttributes(Font* font)
    : TextAttributes(font, System::GetColor(System::Color::Text)) {}

TextAttributes::TextAttributes(Color color)
    : TextAttributes(App::GetCurrent()->GetDefaultFont(), color) {}

TextAttributes::~TextAttributes() {
}

}  // namespace nu
