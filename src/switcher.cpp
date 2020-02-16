#include "switcher.h"

#include <spdlog/spdlog.h>

#include "server.h"
#include "view.h"

namespace lumin {

SwitcherView::SwitcherView()
  : window_(nullptr)
{
}

void SwitcherView::init() {
  window_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_decorated(GTK_WINDOW(window_), false);

  const char* css_data = ".nohighlight { margin: 2px; } .highlight { box-sizing: border-box; border: 2px solid #ddd; border-radius: 5px; } .switcher { background-color: #333; border-radius: 5px; padding: 20px; color: #eee; }";

  auto css = gtk_css_provider_new();
  gtk_css_provider_load_from_data(css, css_data, strlen(css_data), NULL);

  auto screen = gdk_screen_get_default();
  gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(css),
    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  GtkStyleContext *context = gtk_widget_get_style_context(window_);
  gtk_style_context_add_class(context, "switcher");

  box_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_container_add(GTK_CONTAINER(window_), box_);
}

void SwitcherView::clear() {
  GList *children = gtk_container_get_children(GTK_CONTAINER(box_));
  for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
    gtk_container_remove(GTK_CONTAINER(box_), GTK_WIDGET(iter->data));
  }
  g_list_free(children);
}

void SwitcherView::populate(const std::vector<View*>& views) {
  for (auto &view : views) {
    auto inner_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    auto image = gtk_image_new_from_icon_name(view->id().c_str(), GTK_ICON_SIZE_DIALOG);
    gtk_image_set_pixel_size(GTK_IMAGE(image), 128);
    gtk_container_add(GTK_CONTAINER(inner_box), image);

    auto label = gtk_label_new(view->title().c_str());
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_label_set_max_width_chars(GTK_LABEL(label), 15);

    gtk_container_add(GTK_CONTAINER(inner_box), label);
    gtk_widget_set_margin_bottom(label, 5);

    gtk_container_add(GTK_CONTAINER(box_), inner_box);
  }

  gtk_window_resize(GTK_WINDOW(window_), 1, 1);
}

void SwitcherView::highlight(int index)
{
  int highlight_index = 0;
  auto children = gtk_container_get_children(GTK_CONTAINER(box_));
  for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
    auto context = gtk_widget_get_style_context(GTK_WIDGET(iter->data));
    gtk_style_context_remove_class(context, "highlight");
    gtk_style_context_remove_class(context, "nohighlight");

    if (highlight_index++ == index) {
      spdlog::warn("hightlight");
      gtk_style_context_add_class(context, "highlight");
    } else {
      gtk_style_context_add_class(context, "nohighlight");
    }
  }
  g_list_free(children);
}

void SwitcherView::show() {
  gtk_widget_show_all(window_);
}

void SwitcherView::hide() {
  gtk_widget_hide(window_);
}

Switcher::Switcher(Server *server)
  : server_(server)
  , selected_index_(-1)
{
  view_ = std::make_unique<SwitcherView>();
}

void Switcher::init() {
  view_->init();
}

void Switcher::previous() {
  spdlog::warn("previous");
  auto views = server_->views();

  std::vector<View*> interesting_views;

  for (auto &view : views) {
    if (view->id().compare("lumin") == 0) {
      continue;
    }

    interesting_views.push_back(view.get());
  }

  if (interesting_views.empty()) {
    return;
  }

  selected_index_--;

  if (selected_index_ < 0) {
    selected_index_ = interesting_views.size() - 1;
  }

  view_->highlight(selected_index_);
}

void Switcher::show() {
  spdlog::warn("next");
  auto views = server_->views();

  std::vector<View*> interesting_views;

  for (auto &view : views) {
    if (view->id().compare("lumin") == 0) {
      continue;
    }

    interesting_views.push_back(view.get());
  }

  if (interesting_views.empty()) {
    return;
  }

  if (selected_index_ == -1) {
    view_->clear();
    view_->populate(interesting_views);
    view_->show();

    if (interesting_views.size() == 1) {
      selected_index_ = -1;
    } else {
      selected_index_ = 0;
    }
  }

  selected_index_++;

  if (selected_index_ >= (int)interesting_views.size()) {
    selected_index_ = 0;
  }

  spdlog::error("{} {}", interesting_views.size(), selected_index_);
  view_->highlight(selected_index_);
}

void Switcher::hide() {
  if (selected_index_ == -1) {
    return;
  }

  auto views = server_->views();
  std::vector<View*> interesting_views;

  for (auto &view : views) {
    if (view->id().compare("lumin") == 0) {
      continue;
    }

    interesting_views.push_back(view.get());
  }

  View* view = interesting_views[selected_index_];
  server_->focus_view(view);

  view_->hide();
  selected_index_ = -1;
}

}  // namespace lumin
