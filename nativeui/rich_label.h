// Copyright 2019 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#ifndef NATIVEUI_RICH_LABEL_H_
#define NATIVEUI_RICH_LABEL_H_

#include <string>

#include "nativeui/gfx/text.h"
#include "nativeui/view.h"

namespace nu {

class AttributedText;

class NATIVEUI_EXPORT RichLabel : public View {
 public:
  explicit RichLabel(AttributedText* text);

  // View class name.
  static const char kClassName[];

  void SetAttributedText(AttributedText* text);
  AttributedText* GetAttributedText() const { return text_.get(); }

  void SetTextDrawOptions(const TextDrawOptions& options);
  const TextDrawOptions& GetTextDrawOptions() const { return options_; }

  // Private: Mark the yoga node as dirty.
  void MarkDirty();

  // View:
  const char* GetClassName() const override;

 protected:
  ~RichLabel() override;

 private:
  NativeView PlatformCreate();
  void PlatformSetAttributedText(AttributedText* text);
  void PlatformUpdateTextDrawOptions();

  TextDrawOptions options_ = {TextAlign::Center, TextAlign::Center, true};
  scoped_refptr<AttributedText> text_;
};

}  // namespace nu

#endif  // NATIVEUI_RICH_LABEL_H_
