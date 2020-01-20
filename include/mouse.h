#ifndef _MOUSE_H
#define _MOUSE_H

struct wlr_input_device;

class Mouse {

public:

  Mouse(struct wlr_input_device *device);

private:

  struct wlr_input_device *device;

};

#endif
