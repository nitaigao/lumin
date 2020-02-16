#ifndef SWITCHER_VIEW_H_
#define SWITCHER_VIEW_H_

#include <gtk/gtk.h>

#include <vector>

namespace lumin {

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

}  // namespace lumin

#endif  // SWITCHER_VIEW_H_
