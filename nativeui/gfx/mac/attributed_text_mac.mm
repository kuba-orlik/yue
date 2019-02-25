// Copyright 2019 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#include "nativeui/gfx/attributed_text.h"

#import <Cocoa/Cocoa.h>

#include "base/strings/sys_string_conversions.h"
#include "nativeui/gfx/font.h"
#include "nativeui/gfx/geometry/rect_f.h"
#include "nativeui/gfx/geometry/size_f.h"
#include "nativeui/gfx/text.h"

namespace nu {

namespace {

inline int IndexToLength(NSAttributedString* str, int start, int end) {
  return end > -1 ? end - start : [str length] - start;
}

}  // namespace

AttributedText::AttributedText(const std::string& text)
    : text_([[NSMutableAttributedString alloc]
                initWithString:base::SysUTF8ToNSString(text)]) {}

AttributedText::~AttributedText() {
  [text_ release];
}

void AttributedText::PlatformSetFontFor(Font* font, int start, int end) {
  [text_ addAttribute:NSFontAttributeName
                value:font->GetNative()
                range:NSMakeRange(start, IndexToLength(text_, start, end))];
}

void AttributedText::PlatformSetColorFor(Color color, int start, int end) {
  [text_ addAttribute:NSForegroundColorAttributeName
                value:color.ToNSColor()
                range:NSMakeRange(start, IndexToLength(text_, start, end))];
}

RectF AttributedText::GetBoundsFor(const SizeF size,
                                   const TextDrawOptions& options) {
  int draw_options = 0;
  if (options.wrap)
    draw_options |= NSStringDrawingUsesLineFragmentOrigin;
  if (options.ellipsis)
    draw_options |= NSStringDrawingTruncatesLastVisibleLine;
  return RectF([text_ boundingRectWithSize:size.ToCGSize()
                                   options:draw_options]);
}

std::string AttributedText::GetText() const {
  return base::SysNSStringToUTF8([text_ string]);
}

}  // namespace nu
