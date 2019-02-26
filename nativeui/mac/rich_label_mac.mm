// Copyright 2019 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#include "nativeui/rich_label.h"

#include "base/strings/sys_string_conversions.h"
#include "nativeui/gfx/attributed_text.h"
#include "nativeui/gfx/mac/painter_mac.h"
#include "nativeui/mac/nu_private.h"
#include "nativeui/mac/nu_view.h"

@interface NURichLabel : NSView<NUView> {
 @private
  nu::NUPrivate private_;
  nu::Color background_color_;
}
@end

@implementation NURichLabel

- (void)drawRect:(NSRect)dirtyRect {
  nu::PainterMac painter;
  painter.SetColor(background_color_);
  painter.FillRect(nu::RectF(dirtyRect));

  auto* label = static_cast<nu::RichLabel*>([self shell]);
  painter.DrawAttributedText(label->GetAttributedText(),
                             nu::RectF(nu::SizeF([self frame].size)),
                             label->GetTextDrawOptions());
}

- (nu::NUPrivate*)nuPrivate {
  return &private_;
}

- (void)setNUFont:(nu::Font*)font {
}

- (void)setNUColor:(nu::Color)color {
}

- (void)setNUBackgroundColor:(nu::Color)color {
  background_color_ = color;
  [self setNeedsDisplay:YES];
}

- (void)setNUEnabled:(BOOL)enabled {
}

- (BOOL)isNUEnabled {
  return YES;
}

@end

namespace nu {

NativeView RichLabel::PlatformCreate() {
  return [[NURichLabel alloc] init];
}

void RichLabel::PlatformSetAttributedText(AttributedText* text) {
}

void RichLabel::PlatformUpdateTextDrawOptions() {
}

}  // namespace nu
