#include <gtkmm.h>

#include "server.h"

#include <wlr/types/wlr_keyboard.h>
#include <linux/input-event-codes.h>
#include "posix_os.h"

namespace lumin {

class Switcher
{
 public:
  Switcher() { }

 public:
  void init()
  {
    window.set_decorated(false);
    window.set_default_size(200, 200);
  }

  void next()
  {
    if (!window.is_visible()) {
      spdlog::debug("show switcher");
      window.show_all();
    }
  }

  void hide()
  {
    if (window.is_visible()) {
      spdlog::debug("hide switcher");
      window.hide();
    }
  }

  Gtk::Window window;
};


class Shell
{
 public:
  explicit Shell(Server *server)
    : server_(server)
  {
    os_ = std::make_shared<PosixOS>();
  }

 public:
  void run()
  {
    auto app = Gtk::Application::create("org.os.Menu");

    switcher_.init();
    server_->on_key.connect_member(this, &Shell::key_binding);

    Gtk::Window bar;
    bar.set_decorated(false);
    app->run(bar);
  }

  void key_binding(uint keycode, uint modifiers, int state, bool* handled)
  {
    spdlog::debug("key code:{} modifiers:{} state:{}", keycode, modifiers, state);
    if (keycode == KEY_BACKSPACE && modifiers == (WLR_MODIFIER_CTRL | WLR_MODIFIER_ALT)) {
      server_->quit();
    }

    if (keycode == KEY_ENTER && modifiers == WLR_MODIFIER_CTRL && state == 1) {
      os_->execute("tilix");
      *handled = true;
    }

    if (keycode == KEY_TAB && modifiers == WLR_MODIFIER_CTRL && state == 1) {
      switcher_.next();
      *handled = true;
    }

    if (keycode == KEY_LEFTCTRL && state == 0) {
      switcher_.hide();
    }
  }

 private:
  Server *server_;
  Switcher switcher_;
  std::shared_ptr<IOS> os_;
};

}  // namespace lumin

using lumin::Server;
using lumin::Shell;

void shell_thread(Server *server)
{
  gtk_init(0, 0);
  Shell shell(server);
  shell.run();
}

std::thread shell_;

void server_ready(Server *server)
{
  shell_ = std::thread(shell_thread, server);
}

int main(int argc, char *argv[])
{
  Server server;
  server.init();
  server.on_ready.connect(server_ready);
  server.run();
  server.destroy();

  return 0;
}
