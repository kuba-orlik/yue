// Copyright 2016 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#include "base/bind.h"
#include "base/run_loop.h"
#include "nativeui/nativeui.h"
#include "testing/gtest/include/gtest/gtest.h"

class WindowTest : public testing::Test {
 protected:
  void SetUp() override {
    window_ = new nu::Window(nu::Window::Options());
  }

  nu::Lifetime lifetime_;
  nu::State state_;
  scoped_refptr<nu::Window> window_;
};

TEST_F(WindowTest, Bounds) {
  nu::SizeF size(123, 456);
  window_->SetContentSize(size);
  EXPECT_EQ(window_->GetContentSize(), size);
  nu::RectF bounds = window_->GetBounds();
  window_->SetBounds(bounds);
  EXPECT_EQ(window_->GetBounds(), bounds);
}

TEST_F(WindowTest, FramelessWindowBounds) {
  nu::Window::Options options;
  options.frame = false;
  window_ = new nu::Window(options);
  nu::SizeF size(123, 456);
  window_->SetContentSize(size);
  EXPECT_EQ(window_->GetContentSize(), size);
  nu::RectF bounds = window_->GetBounds();
  window_->SetBounds(bounds);
  EXPECT_EQ(window_->GetBounds(), bounds);
}

TEST_F(WindowTest, TransparentWindow) {
  nu::Window::Options options;
  options.frame = false;
  options.transparent = true;
  window_ = new nu::Window(options);
  EXPECT_EQ(window_->IsResizable(), false);
  EXPECT_EQ(window_->IsMaximizable(), false);
}

TEST_F(WindowTest, ContentView) {
  EXPECT_NE(window_->GetContentView(), nullptr);
  scoped_refptr<nu::Container> view(new nu::Container);
  window_->SetContentView(view.get());
  EXPECT_EQ(window_->GetContentView(), view.get());
}

void OnClose(bool* ptr, nu::Window*) {
  *ptr = true;
}

TEST_F(WindowTest, OnClose) {
  bool closed = false;
  window_->on_close.Connect(base::Bind(&OnClose, &closed));
  window_->Close();
  EXPECT_EQ(closed, true);
}

bool ShouldClose(nu::Window*) {
  return false;
}

TEST_F(WindowTest, ShouldClose) {
  bool closed = false;
  window_->on_close.Connect(base::Bind(&OnClose, &closed));
  window_->should_close = base::Bind(&ShouldClose);
  window_->Close();
  EXPECT_EQ(closed, false);
  window_->should_close.Reset();
  window_->Close();
  EXPECT_EQ(closed, true);
}

TEST_F(WindowTest, Visible) {
  window_->SetVisible(true);
  ASSERT_TRUE(window_->IsVisible());
  window_->SetVisible(false);
  ASSERT_FALSE(window_->IsVisible());
}

TEST_F(WindowTest, Resizable) {
  nu::SizeF size(123, 456);
  window_->SetContentSize(size);
  nu::RectF bounds = window_->GetBounds();
  EXPECT_EQ(window_->IsResizable(), true);
  window_->SetResizable(false);
  EXPECT_EQ(window_->IsResizable(), false);
  EXPECT_EQ(window_->GetBounds(), bounds);
  EXPECT_EQ(window_->GetContentSize(), size);
  window_->SetResizable(true);
  EXPECT_EQ(window_->GetBounds(), bounds);
  EXPECT_EQ(window_->GetContentSize(), size);
}

TEST_F(WindowTest, VisibleWindowResizable) {
  nu::SizeF size(123, 456);
  window_->SetContentSize(size);
  EXPECT_EQ(window_->GetContentSize(), size);
  window_->SetResizable(false);
  EXPECT_EQ(window_->GetContentSize(), size);
  window_->SetResizable(true);
  EXPECT_EQ(window_->GetContentSize(), size);
}

TEST_F(WindowTest, FramelessWindowResizable) {
  nu::Window::Options options;
  options.frame = false;
  window_ = new nu::Window(options);
  nu::SizeF size(123, 456);
  window_->SetContentSize(size);
  EXPECT_EQ(window_->GetContentSize(), size);
  window_->SetResizable(false);
  EXPECT_EQ(window_->GetContentSize(), size);
  window_->SetResizable(true);
  EXPECT_EQ(window_->GetContentSize(), size);
}

TEST_F(WindowTest, TransparentWindowResizable) {
  nu::Window::Options options;
  options.frame = false;
  options.transparent = true;
  window_ = new nu::Window(options);
  nu::SizeF size(123, 456);
  window_->SetContentSize(size);
  EXPECT_EQ(window_->GetContentSize(), size);
  window_->SetResizable(false);
  EXPECT_EQ(window_->GetContentSize(), size);
  window_->SetResizable(true);
  EXPECT_EQ(window_->GetContentSize(), size);
}
