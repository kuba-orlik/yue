// Copyright 2019 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#include "nativeui/gfx/attributed_text.h"

#include <gtk/gtk.h>
#include <pango/pango.h>

#include "base/strings/utf_string_conversions.h"
#include "nativeui/gfx/font.h"
#include "nativeui/gfx/geometry/rect_f.h"
#include "nativeui/gfx/geometry/size_f.h"
#include "nativeui/gfx/gtk/text_gtk.h"
#include "nativeui/gfx/text.h"

namespace nu {

namespace {

// Convert the character index to byte index.
guint CharIndexToByteIndex(const base::string16& text, int index) {
  if (index < 0)
    return G_MAXUINT;
  return base::UTF16ToUTF8(text.substr(0, index)).size();
}

void FillPangoAttributeIndex(PangoAttribute* attr, PangoLayout* layout,
                             int start, int end) {
  if (start == 0 && end == -1)  // this is the most common case
    return;
  base::string16 text = base::UTF8ToUTF16(pango_layout_get_text(layout));
  attr->start_index = CharIndexToByteIndex(text, start);
  attr->end_index = CharIndexToByteIndex(text, end);
}

}  // namespace

AttributedText::AttributedText(const std::string& text) {
  static PangoContext* context = nullptr;
  if (!context) {
    context = gdk_pango_context_get_for_screen(gdk_screen_get_default());
    pango_context_set_language(context, pango_language_get_default());
  }
  text_ = pango_layout_new(context);
  // Set text.
  pango_layout_set_text(text_, text.c_str(), text.size());
  // Set attributes.
  PangoAttrList* attrs = pango_attr_list_new();
  pango_layout_set_attributes(text_, attrs);
  pango_attr_list_unref(attrs);
}

AttributedText::~AttributedText() {
  g_object_unref(text_);
}

void AttributedText::PlatformSetFontFor(Font* font, int start, int end) {
  PangoAttribute* font_attr = pango_attr_font_desc_new(font->GetNative());
  FillPangoAttributeIndex(font_attr, text_, start, end);

  PangoAttrList* attrs = pango_layout_get_attributes(text_);
  pango_attr_list_insert(attrs, font_attr);  // ownership taken
}

void AttributedText::PlatformSetColorFor(Color color, int start, int end) {
  PangoAttribute* fg_attr = pango_attr_foreground_new(
      color.r() / 255. * 65535,
      color.g() / 255. * 65535,
      color.b() / 255. * 65535);
  FillPangoAttributeIndex(fg_attr, text_, start, end);

  PangoAttrList* attrs = pango_layout_get_attributes(text_);
  pango_attr_list_insert(attrs, fg_attr);  // ownership taken
}

RectF AttributedText::GetBoundsFor(const SizeF size,
                                   const TextDrawOptions& options) {
  SetupPangoLayout(text_, size, options);
  int width, height;
  pango_layout_get_pixel_size(text_, &width, &height);
  return RectF(0, 0, width, height);
}

std::string AttributedText::GetText() const {
  return pango_layout_get_text(text_);
}

}  // namespace nu
