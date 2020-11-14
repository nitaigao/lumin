#include "shell.h"

#include <wlr/types/wlr_keyboard.h>
#include <linux/input-event-codes.h>

#include <fmt/core.h>
#include <zmqpp/zmqpp.hpp>

#include "rpc.h"
#include "server.h"
#include "posix_os.h"

namespace lumin {

zmqpp::context context;
zmqpp::socket socket(context, zmqpp::socket_type::publish);

Shell::Shell()
{
  rpc_ = std::make_shared<RPC>();
  server_ = std::make_shared<Server>();
  os_ = std::make_shared<PosixOS>();
}

void Shell::run()
{
  socket.bind("tcp://*:4242");

  server_->init();
  server_->on_key.connect_member(this, &Shell::handle_key);

  rpc_->start(server_.get());

  server_->run();
  server_->destroy();
}

void Shell::handle_key(uint keycode, uint modifiers, int state, bool *handled)
{
  if (keycode == KEY_BACKSPACE &&
      modifiers == (WLR_MODIFIER_CTRL | WLR_MODIFIER_ALT) && state == WLR_KEY_PRESSED) {
    server_->quit();
    *handled = true;
    return;
  }

  if (keycode == KEY_ENTER && modifiers == WLR_MODIFIER_CTRL && state == 1) {
    os_->execute("tilix");
    *handled = true;
    return;
  }

  if (keycode == KEY_SPACE && modifiers == WLR_MODIFIER_CTRL && state == 1) {
    os_->execute("launch2");
    *handled = true;
    return;
  }

  if (keycode == KEY_LEFT && modifiers == WLR_MODIFIER_LOGO && state == 1) {
    server_->dock_top_left();
    *handled = true;
    return;
  }

  if (keycode == KEY_RIGHT && modifiers == WLR_MODIFIER_LOGO && state == 1) {
    server_->dock_top_right();
    *handled = true;
    return;
  }

  if (keycode == KEY_UP && modifiers == WLR_MODIFIER_LOGO && state == 1) {
    server_->maximize_top();
    *handled = true;
    return;
  }
}

}  // namespace lumin
