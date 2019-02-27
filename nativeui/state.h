// Copyright 2016 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#ifndef NATIVEUI_STATE_H_
#define NATIVEUI_STATE_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "nativeui/app.h"

#if defined(OS_WIN)
#include <wrl/client.h>
#endif

typedef struct YGConfig *YGConfigRef;

#if defined(OS_WIN)
namespace base {
namespace win {
class ScopedCOMInitializer;
}
}

typedef struct IDWriteFactory IDWriteFactory;
typedef struct ID2D1Factory ID2D1Factory;
#endif  // defined(OS_WIN)

namespace nu {

#if defined(OS_WIN)
class ClassRegistrar;
class GdiplusHolder;
class NativeTheme;
class SubwinHolder;
class ScopedOleInitializer;
class TrayHost;
#endif

class NATIVEUI_EXPORT State {
 public:
  State();
  ~State();

  static State* GetCurrent();

  // Return the instance of App.
  App* GetApp() { return &app_; }

  // Internal classes.
#if defined(OS_WIN)
  void InitializeCOM();
  HWND GetSubwinHolder();
  ClassRegistrar* GetClassRegistrar();
  NativeTheme* GetNativeTheme();
  TrayHost* GetTrayHost();
  UINT GetNextCommandID();
  IDWriteFactory* GetDWriteFactory();
  ID2D1Factory* GetD2D1Factory();
#endif

  // Internal: Return the default yoga config.
  YGConfigRef yoga_config() const { return yoga_config_; }

 private:
  void PlatformInit();

#if defined(OS_WIN)
  std::unique_ptr<base::win::ScopedCOMInitializer> com_initializer_;
  std::unique_ptr<ScopedOleInitializer> ole_initializer_;
  std::unique_ptr<GdiplusHolder> gdiplus_holder_;
  std::unique_ptr<ClassRegistrar> class_registrar_;
  std::unique_ptr<SubwinHolder> subwin_holder_;
  std::unique_ptr<NativeTheme> native_theme_;
  std::unique_ptr<TrayHost> tray_host_;
  Microsoft::WRL::ComPtr<IDWriteFactory> dwrite_factory_;
  Microsoft::WRL::ComPtr<ID2D1Factory> d2d1_factory_;

  // Next ID for custom WM_COMMAND items, the number came from:
  // https://msdn.microsoft.com/en-us/library/11861byt.aspx
  UINT next_command_id_ = 0x8000;
#endif

  // The app instance.
  App app_;

  YGConfigRef yoga_config_;

  DISALLOW_COPY_AND_ASSIGN(State);
};

}  // namespace nu

#endif  // NATIVEUI_STATE_H_
