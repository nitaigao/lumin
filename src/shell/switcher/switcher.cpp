#include "shell/switcher/switcher.h"

#include <spdlog/spdlog.h>

#include "shell/switcher/view.h"

#include "server.h"
#include "view.h"

namespace lumin {

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

void Switcher::next() {
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
