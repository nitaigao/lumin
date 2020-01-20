#include "view.h"

#include <memory>

#include "server.h"

View::View(const Server* server, struct wlr_xdg_surface* surface)
  : server_(server)
  , mapped_(false)
  , xdg_surface_(surface)
  , x(0)
  , y(0) {

}

void View::map() {
  mapped_ = true;
}

bool View::mapped() const {
  return mapped_;
}

void View::focus() const {
  server_->focus_view(this);
}
