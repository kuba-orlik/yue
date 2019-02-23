// Copyright 2019 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#ifndef NATIVEUI_SYSTEM_H_
#define NATIVEUI_SYSTEM_H_

#include "base/macros.h"
#include "nativeui/gfx/color.h"

namespace nu {

class NATIVEUI_EXPORT System {
 public:
  enum class Color {
    Text,
    DisabledText,
  };

  static nu::Color GetColor(Color name);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(System);
};

}  // namespace nu

#endif  // NATIVEUI_SYSTEM_H_
