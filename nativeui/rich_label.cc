// Copyright 2016 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#include "nativeui/rich_label.h"

#include "nativeui/gfx/attributed_text.h"
#include "third_party/yoga/yoga/Yoga.h"

namespace nu {

namespace {

YGSize MeasureRichLabel(YGNodeRef node,
                    float width, YGMeasureMode mode,
                    float height, YGMeasureMode height_mode) {
  auto* label = static_cast<RichLabel*>(YGNodeGetContext(node));
  SizeF size = label->GetAttributedText()
                    ->GetBoundsFor(SizeF(width, height),
                                   label->GetTextDrawOptions()).size();
  size.Enlarge(1, 1);  // leave space for border
  return {std::ceil(size.width()), std::ceil(size.height())};
}

}  // namespace

// static
const char RichLabel::kClassName[] = "RichLabel";

RichLabel::RichLabel(AttributedText* text) {
  TakeOverView(PlatformCreate());
  YGNodeSetMeasureFunc(node(), MeasureRichLabel);
  SetAttributedText(text);
}

RichLabel::~RichLabel() {
}

const char* RichLabel::GetClassName() const {
  return kClassName;
}

void RichLabel::SetAttributedText(AttributedText* text) {
  PlatformSetAttributedText(text);
  text_ = text;
  MarkDirty();
}

void RichLabel::SetTextDrawOptions(const TextDrawOptions& options) {
  options_ = options;
  PlatformUpdateTextDrawOptions();
  MarkDirty();
}

void RichLabel::MarkDirty() {
  YGNodeMarkDirty(node());
  SchedulePaint();
}

}  // namespace nu
