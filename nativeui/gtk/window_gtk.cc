// Copyright 2016 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#include "nativeui/window.h"

#include <gtk/gtk.h>

#include "nativeui/gtk/widget_util.h"
#include "nativeui/menu_bar.h"

namespace nu {

namespace {

// Window private data.
struct NUWindowPrivate {
  Window* delegate = nullptr;
  // Whether window is configured.
  bool is_configured = false;
  // Window state.
  int window_state = 0;
  // Min/max sizes.
  bool needs_to_update_minmax_size = false;
  bool use_content_minmax_size = false;
  SizeF min_size;
  SizeF max_size;
  // Input shape fields.
  bool is_input_shape_set = false;
  bool is_draw_handler_set = false;
  guint draw_handler_id = 0;
};

// Helper to receive private data.
inline NUWindowPrivate* GetPrivate(const Window* window) {
  return static_cast<NUWindowPrivate*>(g_object_get_data(
      G_OBJECT(window->GetNative()), "private"));
}

// User clicks the close button.
gboolean OnClose(GtkWidget* widget, GdkEvent* event, Window* window) {
  if (window->should_close.is_null() || window->should_close.Run(window))
    window->Close();

  // We are destroying the window ourselves, so prevent the default behavior.
  return TRUE;
}

// Window state has changed.
gboolean OnWindowState(GtkWidget* widget, GdkEvent* event,
                       NUWindowPrivate* priv) {
  priv->window_state = event->window_state.new_window_state;
  return FALSE;
}

// Window is unrealized, this is not expected but may happen.
void OnUnrealize(GtkWidget* widget, NUWindowPrivate* priv) {
  priv->is_configured = false;
}

// Window is configured.
gboolean OnConfigure(GtkWidget* widget, GdkEvent* event,
                     NUWindowPrivate* priv) {
  priv->is_configured = true;
  if (priv->needs_to_update_minmax_size) {
    if (priv->use_content_minmax_size)
      priv->delegate->SetContentSizeConstraints(priv->min_size, priv->max_size);
    else
      priv->delegate->SetSizeConstraints(priv->min_size, priv->max_size);
  }
  return FALSE;
}

// Make window support alpha channel for the screen.
void OnScreenChanged(GtkWidget* widget, GdkScreen* old_screen, Window* window) {
  GdkScreen* screen = gtk_widget_get_screen(widget);
  GdkVisual* visual = gdk_screen_get_rgba_visual(screen);
  gtk_widget_set_visual(widget, visual);
}

// Set input shape for frameless transparent window.
gboolean OnDraw(GtkWidget* widget, cairo_t* cr, NUWindowPrivate* priv) {
  cairo_surface_t* surface = cairo_get_target(cr);
  cairo_region_t* region = CreateRegionFromSurface(surface);
  gtk_widget_input_shape_combine_region(widget, region);
  cairo_region_destroy(region);
  // Only handle once.
  priv->is_draw_handler_set = false;
  priv->is_input_shape_set = true;
  g_signal_handler_disconnect(widget, priv->draw_handler_id);
  return FALSE;
}

// Get the height of menubar.
inline int GetMenuBarHeight(const Window* window) {
  int menu_bar_height = 0;
  if (window->GetMenu()) {
    gtk_widget_get_preferred_height(GTK_WIDGET(window->GetMenu()->GetNative()),
                                    &menu_bar_height, nullptr);
  }
  return menu_bar_height;
}

// Return the insets of native window frame.
InsetsF GetNativeFrameInsets(const Window* window, bool include_menu_bar) {
  if (!window->HasFrame())
    return InsetsF();

  // Take menubar as non-client area.
  int menu_bar_height = include_menu_bar ? GetMenuBarHeight(window) : 0;

  // There is no way to know frame size when window is not mapped.
  GdkWindow* gdkwindow = gtk_widget_get_window(GTK_WIDGET(window->GetNative()));
  if (!gdkwindow)
    return InsetsF(menu_bar_height, 0, 0, 0);

  // Get frame size.
  GdkRectangle rect;
  gdk_window_get_frame_extents(gdkwindow, &rect);
  // Subtract gdk window size to get frame insets.
  int x, y, width, height;
  gdk_window_get_geometry(gdkwindow, &x, &y, &width, &height);
  return InsetsF(y - rect.y + menu_bar_height,
                 x - rect.x,
                 (rect.y + rect.height) - (y + height),
                 (rect.x + rect.width) - (x + width));
}

}  // namespace

void Window::PlatformInit(const Options& options) {
  window_ = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));

  NUWindowPrivate* priv = new NUWindowPrivate;
  priv->delegate = this;
  g_object_set_data_full(G_OBJECT(window_), "private", priv, operator delete);

  // Window is not focused by default.
  gtk_window_set_focus_on_map(window_, false);

  // Window events.
  g_signal_connect(window_, "delete-event", G_CALLBACK(OnClose), this);
  g_signal_connect(window_, "window-state-event",
                   G_CALLBACK(OnWindowState), priv);
  g_signal_connect(window_, "unrealize", G_CALLBACK(OnUnrealize), priv);
  g_signal_connect(window_, "configure-event", G_CALLBACK(OnConfigure), priv);

  if (!options.frame) {
    // Rely on client-side decoration to provide window features for frameless
    // window, like resizing and shadows.
    EnableCSD(window_);
  }

  if (options.transparent) {
    // Transparent background.
    gtk_widget_set_app_paintable(GTK_WIDGET(window_), true);
    // Set alpha channel in window.
    OnScreenChanged(GTK_WIDGET(window_), nullptr, this);
    g_signal_connect(window_, "screen-changed",
                     G_CALLBACK(OnScreenChanged), this);
  }

  // Must use a vbox to pack menubar.
  GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(window_), vbox);
}

void Window::PlatformDestroy() {
  if (window_)
    gtk_widget_destroy(GTK_WIDGET(window_));
}

void Window::Close() {
  if (!should_close.is_null() && !should_close.Run(this))
    return;

  on_close.Emit(this);
  gtk_widget_destroy(GTK_WIDGET(window_));

  window_ = nullptr;
}

void Window::PlatformSetContentView(View* view) {
  GtkContainer* vbox = GTK_CONTAINER(gtk_bin_get_child(GTK_BIN(window_)));
  if (content_view_)
    gtk_container_remove(vbox, content_view_->GetNative());
  gtk_container_add(vbox, view->GetNative());
  gtk_box_set_child_packing(GTK_BOX(vbox), view->GetNative(), TRUE, TRUE,
                            0, GTK_PACK_END);

  ForceSizeAllocation(window_, GTK_WIDGET(vbox));

  // For frameless transparent window, we need to set input shape to allow
  // click-through in transparent areas. For best performance we only set
  // input shape the first time when content view is drawn, GTK do redraws very
  // frequently and computing input shape is rather expensive.
  if (IsTransparent() && !HasFrame()) {
    NUWindowPrivate* priv = GetPrivate(this);
    if (!priv->is_draw_handler_set) {
      priv->is_draw_handler_set = true;
      priv->draw_handler_id = g_signal_connect_after(
          G_OBJECT(window_), "draw", G_CALLBACK(OnDraw), priv);
    }
  }
}

void Window::Center() {
  gtk_window_set_position(window_, GTK_WIN_POS_CENTER);
}

void Window::SetContentSize(const SizeF& size) {
  // Menubar is part of client area in GTK.
  ResizeWindow(window_, IsResizable(),
               size.width(), size.height() + GetMenuBarHeight(this));
}

void Window::SetBounds(const RectF& bounds) {
  RectF cbounds(bounds);
  cbounds.Inset(GetNativeFrameInsets(this, false));
  ResizeWindow(window_, IsResizable(), cbounds.width(), cbounds.height());
  gtk_window_move(window_, cbounds.x(), cbounds.y());
}

RectF Window::GetBounds() const {
  GdkWindow* gdkwindow = gtk_widget_get_window(GTK_WIDGET(window_));
  if (gdkwindow && !IsUsingCSD(window_)) {
    // For frameless window using CSD, the window size includes the shadows
    // so the frame extents can not be used.
    GdkRectangle rect;
    gdk_window_get_frame_extents(gdkwindow, &rect);
    return RectF(Rect(rect));
  } else {
    int x, y, width, height;
    gtk_window_get_position(window_, &x, &y);
    gtk_window_get_size(window_, &width, &height);
    return RectF(x, y, width, height);
  }
}

void Window::SetSizeConstraints(const SizeF& min_size, const SizeF& max_size) {
  NUWindowPrivate* priv = GetPrivate(this);
  priv->use_content_minmax_size = false;
  priv->min_size = min_size;
  priv->max_size = max_size;

  // We can not defer the window frame if window is not configured, so update
  // size constraints when window is configured.
  priv->needs_to_update_minmax_size = !priv->is_configured;
  if (!priv->is_configured)
    return;

  GdkGeometry hints = { 0 };
  int flags = 0;
  if (!min_size.IsEmpty()) {
    RectF bounds(min_size);
    if (IsUsingCSD(window_))
      bounds.Inset(-GetClientShadow(window_));
    else
      bounds.Inset(GetNativeFrameInsets(this, false));
    flags |= GDK_HINT_MIN_SIZE;
    hints.min_width = bounds.width();
    hints.min_height = bounds.height();
  }
  if (!max_size.IsEmpty()) {
    RectF bounds(max_size);
    if (IsUsingCSD(window_))
      bounds.Inset(-GetClientShadow(window_));
    else
      bounds.Inset(GetNativeFrameInsets(this, false));
    flags |= GDK_HINT_MAX_SIZE;
    hints.max_width = bounds.width();
    hints.max_height = bounds.height();
  }
  gtk_window_set_geometry_hints(window_, NULL, &hints, GdkWindowHints(flags));
}

std::tuple<SizeF, SizeF> Window::GetSizeConstraints() const {
  NUWindowPrivate* priv = GetPrivate(this);
  if (!priv->use_content_minmax_size)
    return std::make_tuple(priv->min_size, priv->max_size);
  return std::tuple<SizeF, SizeF>();
}

void Window::SetContentSizeConstraints(const SizeF& min_size,
                                       const SizeF& max_size) {
  NUWindowPrivate* priv = GetPrivate(this);
  priv->use_content_minmax_size = true;
  priv->min_size = min_size;
  priv->max_size = max_size;

  // We can not defer the window frame if window is not configured, so update
  // size constraints when window is configured.
  priv->needs_to_update_minmax_size = !priv->is_configured;
  if (!priv->is_configured)
    return;

  GdkGeometry hints = { 0 };
  int flags = 0;
  if (!min_size.IsEmpty()) {
    RectF bounds(min_size);
    bounds.set_height(bounds.height() + GetMenuBarHeight(this));
    if (IsUsingCSD(window_))
      bounds.Inset(-GetClientShadow(window_));
    priv->min_size = bounds.size();
    flags |= GDK_HINT_MIN_SIZE;
    hints.min_width = bounds.width();
    hints.min_height = bounds.height();
  }
  if (!max_size.IsEmpty()) {
    RectF bounds(max_size);
    bounds.set_height(bounds.height() + GetMenuBarHeight(this));
    if (IsUsingCSD(window_))
      bounds.Inset(-GetClientShadow(window_));
    priv->max_size = bounds.size();
    flags |= GDK_HINT_MAX_SIZE;
    hints.max_width = bounds.width();
    hints.max_height = bounds.height();
  }
  gtk_window_set_geometry_hints(window_, NULL, &hints, GdkWindowHints(flags));
}

std::tuple<SizeF, SizeF> Window::GetContentSizeConstraints() const {
  NUWindowPrivate* priv = GetPrivate(this);
  if (priv->use_content_minmax_size)
    return std::make_tuple(priv->min_size, priv->max_size);
  return std::tuple<SizeF, SizeF>();
}

void Window::Activate() {
  gtk_window_present(window_);
}

void Window::Deactivate() {
  gdk_window_lower(gtk_widget_get_window(GTK_WIDGET(window_)));
}

bool Window::IsActive() const {
  return gtk_window_is_active(window_);
}

void Window::SetVisible(bool visible) {
  gtk_widget_set_visible(GTK_WIDGET(window_), visible);
}

bool Window::IsVisible() const {
  return gtk_widget_get_visible(GTK_WIDGET(window_));
}

void Window::SetAlwaysOnTop(bool top) {
  gtk_window_set_keep_above(window_, top);
}

bool Window::IsAlwaysOnTop() const {
  return GetPrivate(this)->window_state & GDK_WINDOW_STATE_ABOVE;
}

void Window::Maximize() {
  gtk_window_maximize(window_);
}

void Window::Unmaximize() {
  gtk_window_unmaximize(window_);
}

bool Window::IsMaximized() const {
  return gtk_window_is_maximized(window_);
}

void Window::Minimize() {
  gtk_window_iconify(window_);
}

void Window::Restore() {
  gtk_window_deiconify(window_);
}

bool Window::IsMinimized() const {
  return GetPrivate(this)->window_state & GDK_WINDOW_STATE_ICONIFIED;
}

void Window::SetResizable(bool resizable) {
  if (resizable == IsResizable())
    return;

  // Current size of content view (gtk_window_get_size is not reliable when
  // window is not realized).
  GdkRectangle alloc;
  GtkWidget* vbox = gtk_bin_get_child(GTK_BIN(window_));
  gtk_widget_get_allocation(vbox, &alloc);

  // Prevent window from changing size when calling gtk_window_set_resizable.
  if (resizable) {
    // Clear the size requests for resizable window, otherwise window would have
    // a minimum size.
    gtk_widget_set_size_request(GTK_WIDGET(window_), -1, -1);
    gtk_widget_set_size_request(gtk_bin_get_child(GTK_BIN(window_)), -1, -1);
    // Window would be resized to default size for resizable window.
    gtk_window_set_default_size(window_, alloc.width, alloc.height);
  } else {
    // Set size requests for unresizable window, otherwise window would be
    // resize to whatever current size request is.
    ResizeWindow(window_, resizable, alloc.width, alloc.height);
  }

  gtk_window_set_resizable(window_, resizable);

  // For transparent window, using CSD means having extra shadow and border, so
  // we only use CSD when window is not resizable.
  if (!HasFrame() && IsTransparent()) {
    if (IsUsingCSD(window_) && !resizable)
      DisableCSD(window_);
    else if (!IsUsingCSD(window_) && resizable)
      EnableCSD(window_);
  }
}

bool Window::IsResizable() const {
  return gtk_window_get_resizable(window_);
}

void Window::SetMaximizable(bool yes) {
  // In theory it is possible to set _NET_WM_ALLOWED_ACTIONS WM hint to
  // implement this, but as I have tried non major desktop environment supports
  // this hint, so on Linux this is simply impossible to implement.
}

bool Window::IsMaximizable() const {
  return IsResizable();
}

void Window::SetMinimizable(bool minimizable) {
  // See SetMaximizable for why this is not implemented.
}

bool Window::IsMinimizable() const {
  return true;
}

void Window::SetMovable(bool movable) {
  // See SetMaximizable for why this is not implemented.
}

bool Window::IsMovable() const {
  return true;
}

void Window::SetBackgroundColor(Color color) {
  GdkRGBA gcolor = color.ToGdkRGBA();
  gtk_widget_override_background_color(GTK_WIDGET(window_),
                                       GTK_STATE_FLAG_NORMAL, &gcolor);
}

void Window::PlatformSetMenu(MenuBar* menu_bar) {
  GtkContainer* vbox = GTK_CONTAINER(gtk_bin_get_child(GTK_BIN(window_)));
  if (menu_bar_)
    gtk_container_remove(vbox, GTK_WIDGET(menu_bar_->GetNative()));
  GtkWidget* menu = GTK_WIDGET(menu_bar->GetNative());
  gtk_container_add(vbox, menu);
  gtk_box_set_child_packing(GTK_BOX(vbox), menu, FALSE, FALSE, 0,
                            GTK_PACK_START);

  // Update the accelerator group.
  if (menu_bar_)
    gtk_window_remove_accel_group(window_,
                                  menu_bar_->accel_manager()->accel_group());
  if (menu_bar)
    gtk_window_add_accel_group(window_,
                               menu_bar->accel_manager()->accel_group());

  ForceSizeAllocation(window_, GTK_WIDGET(vbox));
}

}  // namespace nu
