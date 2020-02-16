#include "shell.h"

#include <gtk/gtk.h>
#include <spdlog/spdlog.h>

#include "shell/switcher/switcher.h"
#include "shell/switcher/view.h"

namespace lumin {

Shell::~Shell() {

}

Shell::Shell(Server *server) {
  switcher_ = std::make_unique<Switcher>(server);
}

void Shell::thread(void *data) {
  Shell *shell = static_cast<Shell*>(data);
  gtk_init(0, NULL);
  shell->init();
  gtk_main();
}

void Shell::init() {
  switcher_->init();
}

void Shell::run() {
  ui_thread_ = std::thread(Shell::thread, this);
}

int switch_app_gui_thread(void *data) {
  auto *switcher = static_cast<Switcher*>(data);
  switcher->next();
  return 0;
}

void Shell::switch_app() {
  g_idle_add(switch_app_gui_thread, switcher_.get());
}

int switch_app_reverse_gui_thread(void *data) {
  auto *switcher = static_cast<Switcher*>(data);
  switcher->previous();
  return 0;
}

void Shell::switch_app_reverse() {
  g_idle_add(switch_app_reverse_gui_thread, switcher_.get());
}

int cancel_activity_gui_thread(void *data) {
  auto *switcher = static_cast<Switcher*>(data);
  switcher->hide();
  return 0;
}

void Shell::cancel_activity() {
  g_idle_add(cancel_activity_gui_thread, switcher_.get());
}

}  // namespace lumin
