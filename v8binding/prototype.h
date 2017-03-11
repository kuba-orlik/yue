// Copyright 2017 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#ifndef V8BINDING_PROTOTYPE_H_
#define V8BINDING_PROTOTYPE_H_

#include <type_traits>

#include "v8binding/prototype_internal.h"

namespace vb {

// Generate prototype for native classes.
template<typename T, typename Enable = void>
struct Prototype;

// Create prototype for RefCounted classes.
template<typename T>
struct Prototype<T, typename std::enable_if<std::is_base_of<
                        base::subtle::RefCountedBase, T>::value>::type> {
  // Get the constructor of the prototype.
  static inline v8::Local<v8::Function> Get(v8::Local<v8::Context> context) {
    return internal::GetConstructor<T>(context);
  }

  // Create an instance of T and store it in an v8::Object.
  template<typename... ArgTypes>
  static v8::Local<v8::Value> NewInstance(v8::Local<v8::Context> context,
                                          const ArgTypes&... args) {
    v8::Isolate* isolate = context->GetIsolate();
    auto result = internal::CallConstructor<T>(context);
    if (result.IsEmpty())
      return v8::Null(isolate);
    // Store the new instance in the object.
    v8::Local<v8::Object> obj = result.ToLocalChecked();
    new internal::RefPtrObjectTracker<T>(isolate, obj, new T(args...));
    return obj;
  }
};

// The default type information for RefCounted class.
template<typename T>
struct Type<T*, typename std::enable_if<std::is_base_of<
                    base::subtle::RefCountedBase, T>::value>::type> {
  static constexpr const char* name = Type<T>::name;
  static v8::Local<v8::Value> ToV8(v8::Local<v8::Context> context, T* ptr) {
    v8::Isolate* isolate = context->GetIsolate();
    if (!ptr)
      return v8::Null(isolate);
    // Whether there is already a wrapper for |ptr|.
    auto wrapper = PerIsolateData::Get(isolate)->GetObjectTracker(ptr);
    if (wrapper)
      return wrapper->GetHandle();
    // If not create a new wrapper for it.
    auto result = internal::CallConstructor<T>(context);
    if (result.IsEmpty())
      return v8::Null(isolate);
    v8::Local<v8::Object> obj = result.ToLocalChecked();
    new internal::RefPtrObjectTracker<T>(isolate, obj, ptr);
    return obj;
  }
  static bool FromV8(v8::Local<v8::Context> context,
                     v8::Local<v8::Value> value,
                     T** out) {
    // Verify the type.
    if (!value->IsObject())
      return false;
    v8::Local<v8::Object> obj = v8::Local<v8::Object>::Cast(value);
    if (obj->InternalFieldCount() != 1)
      return false;
    // Convert pointer to actual class.
    *out = static_cast<T*>(obj->GetAlignedPointerFromInternalField(0));
    return *out != nullptr;
  }
};

// Create prototype for classes that produce WeakPtr.
template<typename T>
struct Prototype<T, typename std::enable_if<std::is_base_of<
                        base::internal::WeakPtrBase,
                        decltype(((T*)nullptr)->GetWeakPtr())>::value>::type> {
  // Get the constructor of the prototype.
  static inline v8::Local<v8::Function> Get(v8::Local<v8::Context> context) {
    return internal::GetConstructor<T>(context);
  }
};

// The default type information for WeakPtr class.
template<typename T>
struct Type<T*, typename std::enable_if<std::is_base_of<
                    base::internal::WeakPtrBase,
                    decltype(((T*)nullptr)->GetWeakPtr())>::value>::type> {
  static constexpr const char* name = Type<T>::name;
  static v8::Local<v8::Value> ToV8(v8::Local<v8::Context> context, T* ptr) {
    v8::Isolate* isolate = context->GetIsolate();
    if (!ptr)
      return v8::Null(isolate);
    // Do not cache the pointer of WeakPtr, because the pointer may point to a
    // variable on stack, which can have same address with previous variable on
    // the stack.
    auto result = internal::CallConstructor<T>(context);
    if (result.IsEmpty())
      return v8::Null(isolate);
    // Store the new instance in the object.
    v8::Local<v8::Object> obj = result.ToLocalChecked();
    new internal::WeakPtrObjectTracker<T>(isolate, obj, ptr->GetWeakPtr());
    return obj;
  }
  static bool FromV8(v8::Local<v8::Context> context,
                     v8::Local<v8::Value> value,
                     T** out) {
    // Verify the type.
    if (!value->IsObject())
      return false;
    v8::Local<v8::Object> obj = v8::Local<v8::Object>::Cast(value);
    if (obj->InternalFieldCount() != 1)
      return false;
    // Convert pointer to actual class.
    auto* ptr = static_cast<internal::WeakPtrObjectTracker<T>*>(
        obj->GetAlignedPointerFromInternalField(0));
    if (!ptr || !ptr->Get())
      return false;
    *out = ptr->Get();
    return true;
  }
};

}  // namespace vb

#endif  // V8BINDING_PROTOTYPE_H_
