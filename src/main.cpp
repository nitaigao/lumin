extern "C" {
  #include <unistd.h>
  #include <wayland-server-core.h>
  #include <wlr/backend.h>
  #include <wlr/xwayland.h>
  #include <wlr/render/wlr_renderer.h>
  #include <wlr/types/wlr_cursor.h>
  #include <wlr/types/wlr_compositor.h>
  #include <wlr/types/wlr_data_device.h>
  #include <wlr/types/wlr_input_device.h>
  #include <wlr/types/wlr_keyboard.h>
  #include <wlr/types/wlr_matrix.h>
  #include <wlr/types/wlr_output.h>
  #include <wlr/types/wlr_output_layout.h>
  #include <wlr/types/wlr_pointer.h>
  #include <wlr/types/wlr_seat.h>
  #include <wlr/types/wlr_xcursor_manager.h>
  #include <wlr/types/wlr_xdg_shell.h>
  #include <wlr/util/log.h>
  #include <xkbcommon/xkbcommon.h>
}

#include <iostream>

#include "turbo_server.h"
#include "turbo_keyboard.h"
#include "turbo_view.h"
#include "turbo_output.h"

static void new_input_notify(wl_listener *listener, void *data) {
  /* This event is raised by the backend when a new input device becomes
   * available. */
  turbo_server *server = wl_container_of(listener, server, new_input);
  auto device = static_cast<struct wlr_input_device *>(data);

  switch (device->type) {
  case WLR_INPUT_DEVICE_KEYBOARD:
    server->new_keyboard(device);
    break;

  case WLR_INPUT_DEVICE_POINTER:
    server->new_pointer(device);
    break;

  default:
    break;

  }
  /* We need to let the wlr_seat know what our capabilities are, which is
   * communiciated to the client. In TinyWL we always have a cursor, even if
   * there are no pointer devices, so we always include that capability. */
  uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
  if (!wl_list_empty(&server->keyboards)) {
    caps |= WL_SEAT_CAPABILITY_KEYBOARD;
  }

  wlr_seat_set_capabilities(server->seat, caps);
}

static void seat_request_cursor_notify(wl_listener *listener, void *data) {
  turbo_server *server = wl_container_of(listener, server, request_cursor);
  /* This event is rasied by the seat when a client provides a cursor image */
  auto event = static_cast<wlr_seat_pointer_request_set_cursor_event*>(data);
  wlr_seat_client *focused_client = server->seat->pointer_state.focused_client;
  /* This can be sent by any client, so we check to make sure this one is
   * actually has pointer focus first. */
  if (focused_client == event->seat_client) {
    /* Once we've vetted the client, we can tell the cursor to use the
     * provided surface as the cursor image. It will set the hardware cursor
     * on the output that it's currently on and continue to do so as the
     * cursor moves between outputs. */
    wlr_cursor_set_surface(server->cursor, event->surface, event->hotspot_x, event->hotspot_y);
  }
}

static void cursor_motion_notify(wl_listener *listener, void *data) {
  /* This event is forwarded by the cursor when a pointer emits a _relative_
   * pointer motion event (i.e. a delta) */
  turbo_server *server = wl_container_of(listener, server, cursor_motion);
  auto event = static_cast<struct wlr_event_pointer_motion*>(data);
  /* The cursor doesn't move unless we tell it to. The cursor automatically
   * handles constraining the motion to the output layout, as well as any
   * special configuration applied for the specific input device which
   * generated the event. You can pass NULL for the device if you want to move
   * the cursor around without any input. */
  wlr_cursor_move(server->cursor, event->device, event->delta_x, event->delta_y);
  server->process_cursor_motion(event->time_msec);
}

static void cursor_motion_absolute_notify(wl_listener *listener, void *data) {
  /* This event is forwarded by the cursor when a pointer emits an _absolute_
   * motion event, from 0..1 on each axis. This happens, for example, when
   * wlroots is running under a Wayland window rather than KMS+DRM, and you
   * move the mouse over the window. You could enter the window from any edge,
   * so we have to warp the mouse there. There is also some hardware which
   * emits these events. */
  turbo_server *server = wl_container_of(listener, server, cursor_motion_absolute);
  auto event = static_cast<struct wlr_event_pointer_motion_absolute*>(data);
  wlr_cursor_warp_absolute(server->cursor, event->device, event->x, event->y);
  server->process_cursor_motion(event->time_msec);
}

static void cursor_button_notify(wl_listener *listener, void *data) {
  /* This event is forwarded by the cursor when a pointer emits a button
   * event. */
  turbo_server *server = wl_container_of(listener, server, cursor_button);
  auto event = static_cast<struct wlr_event_pointer_button*>(data);
  /* Notify the client with pointer focus that a button press has occurred */
  wlr_seat_pointer_notify_button(server->seat, event->time_msec, event->button, event->state);

  double sx, sy;
  wlr_surface *surface;
  turbo_view *view = server->desktop_view_at(server->cursor->x, server->cursor->y, &surface, &sx, &sy);

  if (event->state == WLR_BUTTON_RELEASED) {
    /* If you released any buttons, we exit interactive move/resize mode. */
    server->cursor_mode = TURBO_CURSOR_PASSTHROUGH;
  } else {
    if (view != NULL) {
      /* Focus that client if the button was _pressed_ */
      view->focus_view(surface);
    }
  }
}

static void cursor_axis_notify(wl_listener *listener, void *data) {
  /* This event is forwarded by the cursor when a pointer emits an axis event,
   * for example when you move the scroll wheel. */
  turbo_server *server = wl_container_of(listener, server, cursor_axis);
  auto event = static_cast<wlr_event_pointer_axis*>(data);
  /* Notify the client with pointer focus of the axis event. */
  wlr_seat_pointer_notify_axis(server->seat, event->time_msec, event->orientation,
    event->delta, event->delta_discrete, event->source);
}

static void cursor_frame_notify(wl_listener *listener, void *data) {
  /* This event is forwarded by the cursor when a pointer emits an frame
   * event. Frame events are sent after regular pointer events to group
   * multiple events together. For instance, two axis events may happen at the
   * same time, in which case a frame event won't be sent in between. */
  turbo_server *server = wl_container_of(listener, server, cursor_frame);
  /* Notify the client with pointer focus of the frame event. */
  wlr_seat_pointer_notify_frame(server->seat);
}

/* Used to move all of the data necessary to render a surface from the top-level
 * frame handler to the per-surface render function. */
struct render_data {
  wlr_output *output;
  wlr_renderer *renderer;
  turbo_view *view;
  timespec *when;
};

static void render_surface(wlr_surface *surface, int sx, int sy, void *data) {
  if (surface == NULL) {
    return;
  }
  /* This function is called for every surface that needs to be rendered. */
  auto rdata = static_cast<struct render_data*>(data);
  turbo_view *view = rdata->view;
  struct wlr_output *output = rdata->output;

  /* We first obtain a wlr_texture, which is a GPU resource. wlroots
   * automatically handles negotiating these with the client. The underlying
   * resource could be an opaque handle passed from the client, or the client
   * could have sent a pixel buffer which we copied to the GPU, or a few other
   * means. You don't have to worry about this, wlroots takes care of it. */
  struct wlr_texture *texture = wlr_surface_get_texture(surface);
  if (texture == NULL) {
    return;
  }

  /* The view has a position in layout coordinates. If you have two displays,
   * one next to the other, both 1080p, a view on the rightmost display might
   * have layout coordinates of 2000,100. We need to translate that to
   * output-local coordinates, or (2000 - 1920). */
  double ox = 0;
  double oy = 0;
  wlr_output_layout_output_coords(view->server->output_layout, output, &ox, &oy);
  ox += view->x + sx;
  oy += view->y + sy;

  float scale = output->scale;

  // let XWayland clients scale themselves
  if (view->surface_type == TURBO_XWAYLAND_SURFACE) {
    scale = 1;
  }

  /* We also have to apply the scale factor for HiDPI outputs. This is only
   * part of the puzzle, TinyWL does not fully support HiDPI. */
  struct wlr_box box = {
    .x = (int)ox * (int)scale,
    .y = (int)oy * (int)scale,
    .width = surface->current.width * (int)scale,
    .height = surface->current.height * (int)scale,
  };

  /*
   * Those familiar with OpenGL are also familiar with the role of matricies
   * in graphics programming. We need to prepare a matrix to render the view
   * with. wlr_matrix_project_box is a helper which takes a box with a desired
   * x, y coordinates, width and height, and an output geometry, then
   * prepares an orthographic projection and multiplies the necessary
   * transforms to produce a model-view-projection matrix.
   *
   * Naturally you can do this any way you like, for example to make a 3D
   * compositor.
   */
  float matrix[9];
  enum wl_output_transform transform = wlr_output_transform_invert(surface->current.transform);
  wlr_matrix_project_box(matrix, &box, transform, 0, output->transform_matrix);

  /* This takes our matrix, the texture, and an alpha, and performs the actual
   * rendering on the GPU. */
  wlr_render_texture_with_matrix(rdata->renderer, texture, matrix, 1);

  /* This lets the client know that we've displayed that frame and it can
   * prepare another one now if it likes. */
  wlr_surface_send_frame_done(surface, rdata->when);
}

static void output_frame_notify(wl_listener *listener, void *data) {
  /* This function is called every time an output is ready to display a frame,
   * generally at the output's refresh rate (e.g. 60Hz). */
  turbo_output *output = wl_container_of(listener, output, frame);
  wlr_renderer *renderer = output->server->renderer;

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);

  /* wlr_output_attach_render makes the OpenGL context current. */
  if (!wlr_output_attach_render(output->wlr_output, NULL)) {
    return;
  }

  /* The "effective" resolution can change if you rotate your outputs. */
  // int width, height;
  // wlr_output_effective_resolution(output->wlr_output, &width, &height);

  /* Begin the renderer (calls glViewport and some other GL sanity checks) */
  wlr_renderer_begin(renderer, output->wlr_output->width, output->wlr_output->height);

  float color[4] = {1.0, 0.0, 0.0, 1.0};
  wlr_renderer_clear(renderer, color);

  /* Each subsequent window we render is rendered on top of the last. Because
   * our view list is ordered front-to-back, we iterate over it backwards. */
  turbo_view *view;
  wl_list_for_each_reverse(view, &output->server->views, link) {
    if (!view->mapped) {
      /* An unmapped view should not be rendered. */
      continue;
    }

    struct render_data rdata = {
      .output = output->wlr_output,
      .renderer = renderer,
      .view = view,
      .when = &now
    };

    if (view->surface_type == TURBO_XWAYLAND_SURFACE && view->xwayland_surface->surface) {
      wlr_surface_for_each_surface(view->xwayland_surface->surface, render_surface, &rdata);
    }

    if (view->surface_type == TURBO_XDG_SURFACE && view->xdg_surface->surface) {
      /* This calls our render_surface function for each surface among the
      * xdg_surface's toplevel and popups. */
      wlr_xdg_surface_for_each_surface(view->xdg_surface, render_surface, &rdata);
    }
  }

  /* Hardware cursors are rendered by the GPU on a separate plane, and can be
   * moved around without re-rendering what's beneath them - which is more
   * efficient. However, not all hardware supports hardware cursors. For this
   * reason, wlroots provides a software fallback, which we ask it to render
   * here. wlr_cursor handles configuring hardware vs software cursors for you,
   * and this function is a no-op when hardware cursors are in use. */
  wlr_output_render_software_cursors(output->wlr_output, NULL);

  /* Conclude rendering and swap the buffers, showing the final frame
   * on-screen. */
  wlr_renderer_end(renderer);
  wlr_output_commit(output->wlr_output);
}

static void new_output_notify(wl_listener *listener, void *data) {
  /* This event is rasied by the backend when a new output (aka a display or
   * monitor) becomes available. */
  turbo_server *server = wl_container_of(listener, server, new_output);
  auto wlr_output = static_cast<struct wlr_output*>(data);

  /* Some backends don't have modes. DRM+KMS does, and we need to set a mode
   * before we can use the output. The mode is a tuple of (width, height,
   * refresh rate), and each monitor supports only a specific set of modes. We
   * just pick the monitor's preferred mode, a more sophisticated compositor
   * would let the user configure it. */
  if (!wl_list_empty(&wlr_output->modes)) {
    struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
    wlr_output_set_mode(wlr_output, mode);
    wlr_output_enable(wlr_output, true);
    if (!wlr_output_commit(wlr_output)) {
      return;
    }
  }

  if (strcmp(wlr_output->name, "X11-1") == 0) {
    wlr_output_set_scale(wlr_output, 3);
  }

  if (strcmp(wlr_output->name, "eDP-1") == 0) {
    wlr_output_set_scale(wlr_output, 3);
  }

  if (strcmp(wlr_output->name, "DP-0") == 0
   || strcmp(wlr_output->name, "DP-1") == 0
   || strcmp(wlr_output->name, "DP-2") == 0) {
    wlr_output_set_scale(wlr_output, 2);
  }

  /* Allocates and configures our state for this output */
  turbo_output *output = new turbo_output();
  output->wlr_output = wlr_output;
  output->server = server;

  /* Sets up a listener for the frame notify event. */
  output->frame.notify = output_frame_notify;
  wl_signal_add(&wlr_output->events.frame, &output->frame);

  /* Adds this to the output layout. The add_auto function arranges outputs
   * from left-to-right in the order they appear. A more sophisticated
   * compositor would let the user configure the arrangement of outputs in the
   * layout. */
  wlr_output_layout_add_auto(server->output_layout, wlr_output);

  /* Creating the global adds a wl_output global to the display, which Wayland
   * clients can see to find out information about the output (such as
   * DPI, scale factor, manufacturer, etc). */
  wlr_output_create_global(wlr_output);

  wl_list_insert(&server->outputs, &output->link);
}

static void xdg_surface_map_notify(wl_listener *listener, void *data) {
  /* Called when the surface is mapped, or ready to display on-screen. */
  turbo_view *view = wl_container_of(listener, view, map);
  view->mapped = true;
  if (view->surface_type == TURBO_XWAYLAND_SURFACE) {
    view->focus_view(view->xwayland_surface->surface);
  }

  if (view->surface_type == TURBO_XDG_SURFACE) {
    view->focus_view(view->xdg_surface->surface);
  }
}

static void xdg_surface_unmap_notify(wl_listener *listener, void *data) {
  /* Called when the surface is unmapped, and should no longer be shown. */
  turbo_view *view = wl_container_of(listener, view, unmap);
  view->mapped = false;
}

static void xdg_surface_destroy_notify(wl_listener *listener, void *data) {
  /* Called when the surface is destroyed and should never be shown again. */
  turbo_view *view = wl_container_of(listener, view, destroy);
  wl_list_remove(&view->link);
  free(view);
}

static void xdg_toplevel_request_move_notify(wl_listener *listener, void *data) {
  /* This event is raised when a client would like to begin an interactive
   * move, typically because the user clicked on their client-side
   * decorations. Note that a more sophisticated compositor should check the
   * provied serial against a list of button press serials sent to this
   * client, to prevent the client from requesting this whenever they want. */
  turbo_view *view = wl_container_of(listener, view, request_move);
  view->begin_interactive(TURBO_CURSOR_MOVE, 0);
}

static void xdg_toplevel_request_resize_notify(wl_listener *listener, void *data) {
  /* This event is raised when a client would like to begin an interactive
   * resize, typically because the user clicked on their client-side
   * decorations. Note that a more sophisticated compositor should check the
   * provied serial against a list of button press serials sent to this
   * client, to prevent the client from requesting this whenever they want. */
  auto event = static_cast<wlr_xdg_toplevel_resize_event*>(data);
  turbo_view *view = wl_container_of(listener, view, request_resize);
  view->begin_interactive(TURBO_CURSOR_RESIZE, event->edges);
}

static void xdg_toplevel_request_maximize_notify(wl_listener *listener, void *data) {
  turbo_view *view = wl_container_of(listener, view, request_maximize);
  view->toggle_maximize();
}

static void xwayland_surface_request_configure_notify(wl_listener *listener, void *data) {
  turbo_view_xwayland *view = wl_container_of(listener, view, request_maximize);
  auto event = static_cast<wlr_xwayland_surface_configure_event*>(data);
  view->x = event->x;
  view->y = event->y;
  wlr_xwayland_surface_configure(event->surface, event->x, event->y, event->width, event->height);
}


static void new_xwayland_surface_notify(wl_listener *listener, void *data) {
  wlr_log(WLR_INFO, "new_xwayland_surface_notify");

  turbo_server *server = wl_container_of(listener, server, new_xwayland_surface);
  auto xwayland_surface = static_cast<wlr_xwayland_surface*>(data);

  turbo_view_xwayland *view = new turbo_view_xwayland();

  view->x = 100;
  view->y = 100;
  view->server = server;
  view->xwayland_surface = xwayland_surface;
  view->surface_type = TURBO_XWAYLAND_SURFACE;

  view->map.notify = xdg_surface_map_notify;
  wl_signal_add(&xwayland_surface->events.map, &view->map);

  view->unmap.notify = xdg_surface_map_notify;
  wl_signal_add(&xwayland_surface->events.unmap, &view->unmap);

  view->destroy.notify = xdg_surface_destroy_notify;
  wl_signal_add(&xwayland_surface->events.destroy, &view->destroy);

  view->request_configure.notify = xwayland_surface_request_configure_notify;
  wl_signal_add(&xwayland_surface->events.request_configure, &view->request_configure);

  view->request_move.notify = xdg_toplevel_request_move_notify;
  wl_signal_add(&xwayland_surface->events.request_move, &view->request_move);

  view->request_resize.notify = xdg_toplevel_request_resize_notify;
  wl_signal_add(&xwayland_surface->events.request_resize, &view->request_resize);

  view->request_maximize.notify = xdg_toplevel_request_maximize_notify;
  wl_signal_add(&xwayland_surface->events.request_maximize, &view->request_maximize);

   /* Add it to the list of views. */
  wl_list_insert(&server->views, &view->link);
}

static void new_xdg_surface_notify(wl_listener *listener, void *data) {
  /* This event is raised when wlr_xdg_shell receives a new xdg surface from a
   * client, either a toplevel (application window) or popup. */
  turbo_server *server = wl_container_of(listener, server, new_xdg_surface);
  auto xdg_surface = static_cast<wlr_xdg_surface*>(data);
  if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
    return;
  }

  /* Allocate a turbo_view for this surface */
  turbo_view *view = new turbo_view_xdg();
  view->x = 0;
  view->y = 0;
  view->server = server;
  view->xdg_surface = xdg_surface;
  view->surface_type = TURBO_XDG_SURFACE;

  /* Listen to the various events it can emit */
  view->map.notify = xdg_surface_map_notify;
  wl_signal_add(&xdg_surface->events.map, &view->map);

  view->unmap.notify = xdg_surface_unmap_notify;
  wl_signal_add(&xdg_surface->events.unmap, &view->unmap);

  view->destroy.notify = xdg_surface_destroy_notify;
  wl_signal_add(&xdg_surface->events.destroy, &view->destroy);

  /* cotd */
  struct wlr_xdg_toplevel *toplevel = xdg_surface->toplevel;

  view->request_move.notify = xdg_toplevel_request_move_notify;
  wl_signal_add(&toplevel->events.request_move, &view->request_move);

  view->request_resize.notify = xdg_toplevel_request_resize_notify;
  wl_signal_add(&toplevel->events.request_resize, &view->request_resize);

  view->request_maximize.notify = xdg_toplevel_request_maximize_notify;
  wl_signal_add(&toplevel->events.request_maximize, &view->request_maximize);

  /* Add it to the list of views. */
  wl_list_insert(&server->views, &view->link);
}

int main(int argc, char *argv[]) {
  // wlr_log_init(WLR_DEBUG, NULL);

  turbo_server server;
  /* The Wayland display is managed by libwayland. It handles accepting
   * clients from the Unix socket, manging Wayland globals, and so on. */
  server.wl_display = wl_display_create();
  /* The backend is a wlroots feature which abstracts the underlying input and
   * output hardware. The autocreate option will choose the most suitable
   * backend based on the current environment, such as opening an X11 window
   * if an X11 server is running. The NULL argument here optionally allows you
   * to pass in a custom renderer if wlr_renderer doesn't meet your needs. The
   * backend uses the renderer, for example, to fall back to software cursors
   * if the backend does not support hardware cursors (some older GPUs
   * don't). */
  server.backend = wlr_backend_autocreate(server.wl_display, NULL);

  /* If we don't provide a renderer, autocreate makes a GLES2 renderer for us.
   * The renderer is responsible for defining the various pixel formats it
   * supports for shared memory, this configures that for clients. */
  server.renderer = wlr_backend_get_renderer(server.backend);
  wlr_renderer_init_wl_display(server.renderer, server.wl_display);

  /* This creates some hands-off wlroots interfaces. The compositor is
   * necessary for clients to allocate surfaces and the data device manager
   * handles the clipboard. Each of these wlroots interfaces has room for you
   * to dig your fingers in and play with their behavior if you want. */
  wlr_compositor *compositor = wlr_compositor_create(server.wl_display, server.renderer);
  // wlr_data_device_manager_create(server.wl_display);

  /* Creates an output layout, which a wlroots utility for working with an
   * arrangement of screens in a physical layout. */
  server.output_layout = wlr_output_layout_create();

  /* Configure a listener to be notified when new outputs are available on the
   * backend. */
  wl_list_init(&server.outputs);
  server.new_output.notify = new_output_notify;
  wl_signal_add(&server.backend->events.new_output, &server.new_output);

  /* Set up our list of views and the xdg-shell. The xdg-shell is a Wayland
   * protocol which is used for application windows. For more detail on
   * shells, refer to my article:
   *
   * https://drewdevault.com/2018/07/29/Wayland-shells.html
   */
  wl_list_init(&server.views);
  server.xdg_shell = wlr_xdg_shell_create(server.wl_display);
  server.new_xdg_surface.notify = new_xdg_surface_notify;
  wl_signal_add(&server.xdg_shell->events.new_surface, &server.new_xdg_surface);

  /*
   * Creates a cursor, which is a wlroots utility for tracking the cursor
   * image shown on screen.
   */
  server.cursor = wlr_cursor_create();
  wlr_cursor_attach_output_layout(server.cursor, server.output_layout);

  /* Creates an xcursor manager, another wlroots utility which loads up
   * Xcursor themes to source cursor images from and makes sure that cursor
   * images are available at all scale factors on the screen (necessary for
   * HiDPI support). We add a cursor theme at scale factor 1 to begin with. */
  server.cursor_mgr = wlr_xcursor_manager_create(NULL, 24);
  wlr_xcursor_manager_load(server.cursor_mgr, 1);
  wlr_xcursor_manager_load(server.cursor_mgr, 2);
  wlr_xcursor_manager_load(server.cursor_mgr, 3);

  /*
   * wlr_cursor *only* displays an image on screen. It does not move around
   * when the pointer moves. However, we can attach input devices to it, and
   * it will generate aggregate events for all of them. In these events, we
   * can choose how we want to process them, forwarding them to clients and
   * moving the cursor around. More detail on this process is described in my
   * input handling blog post:
   *
   * https://drewdevault.com/2018/07/17/Input-handling-in-wlroots.html
   *
   * And more comments are sprinkled throughout the notify functions above.
   */
  server.cursor_motion.notify = cursor_motion_notify;
  wl_signal_add(&server.cursor->events.motion, &server.cursor_motion);

  server.cursor_motion_absolute.notify = cursor_motion_absolute_notify;
  wl_signal_add(&server.cursor->events.motion_absolute, &server.cursor_motion_absolute);

  server.cursor_button.notify = cursor_button_notify;
  wl_signal_add(&server.cursor->events.button, &server.cursor_button);

  server.cursor_axis.notify = cursor_axis_notify;
  wl_signal_add(&server.cursor->events.axis, &server.cursor_axis);

  server.cursor_frame.notify = cursor_frame_notify;
  wl_signal_add(&server.cursor->events.frame, &server.cursor_frame);

  /*
   * Configures a seat, which is a single "seat" at which a user sits and
   * operates the computer. This conceptually includes up to one keyboard,
   * pointer, touch, and drawing tablet device. We also rig up a listener to
   * let us know when new input devices are available on the backend.
   */
  wl_list_init(&server.keyboards);

  server.new_input.notify = new_input_notify;
  wl_signal_add(&server.backend->events.new_input, &server.new_input);

  server.seat = wlr_seat_create(server.wl_display, "seat0");

  server.request_cursor.notify = seat_request_cursor_notify;
  wl_signal_add(&server.seat->events.request_set_cursor, &server.request_cursor);

  /* Add a Unix socket to the Wayland display. */
  const char *socket = wl_display_add_socket_auto(server.wl_display);
  if (!socket) {
    wlr_backend_destroy(server.backend);
    return 1;
  }

  /* Start the backend. This will enumerate outputs and inputs, become the DRM
   * master, etc */
  if (!wlr_backend_start(server.backend)) {
    wlr_backend_destroy(server.backend);
    wl_display_destroy(server.wl_display);
    return 1;
  }

  /* Set the WAYLAND_DISPLAY environment variable to our socket and run the
   * startup command if requested. */
  setenv("WAYLAND_DISPLAY", socket, true);

  wl_display_init_shm(server.wl_display);
  wlr_data_device_manager_create(server.wl_display);
  server.xwayland = wlr_xwayland_create(server.wl_display, compositor, true);
  wlr_xwayland_set_seat(server.xwayland, server.seat);

  server.new_xwayland_surface.notify = new_xwayland_surface_notify;
  wl_signal_add(&server.xwayland->events.new_surface, &server.new_xwayland_surface);
  wlr_log(WLR_INFO, "Running XWayland on DISPLAY=%s", server.xwayland->display_name);

  /* Run the Wayland event loop. This does not return until you exit the
   * compositor. Starting the backend rigged up all of the necessary event
   * loop configuration to listen to libinput events, DRM events, generate
   * frame events at the refresh rate, and so on. */
  wlr_log(WLR_INFO, "Running Wayland compositor on WAYLAND_DISPLAY=%s", socket);
  wl_display_run(server.wl_display);

  /* Once wl_display_run returns, we shut down the server. */
  wl_display_destroy_clients(server.wl_display);
  wl_display_destroy(server.wl_display);

  return 0;
}
