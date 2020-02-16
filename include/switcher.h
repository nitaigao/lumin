#ifndef SWITCHER_H_
#define SWITCHER_H_

#include <gtk/gtk.h>

#include <memory>
#include <vector>

namespace lumin {

class Server;
class View;

class SwitcherView {
 public:
  SwitcherView();
 public:
  void init();
  void clear();
  void populate(const std::vector<View*>& views);
  void show();
  void hide();
  void highlight(int index);
 private:
  GtkWidget *window_;
  GtkWidget *box_;
};

class Switcher {
 public:
  Switcher(Server *server);
 public:
  void init();
  void show();
  void previous();
  void hide();
 private:
  std::unique_ptr<SwitcherView> view_;
  Server *server_;
  int selected_index_;
};

}  // namespace lumin

#endif  // SWITCHER_H_
