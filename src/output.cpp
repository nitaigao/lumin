#include "output.h"

extern "C" {
  #include <unistd.h>
  #include <wayland-server-core.h>
  #include <wlr/backend.h>
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
  #include <wlr/types/wlr_output_damage.h>
  #include <wlr/util/log.h>
  #include <wlr/util/region.h>
  #include <wlr/xwayland.h>
  #include <wlr/backend/libinput.h>
  #include <xkbcommon/xkbcommon.h>
}

#include "view.h"
#include "controller.h"

struct render_data {
  wlr_output *output;
  wlr_renderer *renderer;
  View *view;
  timespec *when;
  wlr_output_layout *layout;
  pixman_region32_t *buffer_damage;
};

struct damage_iterator_data {
  const View *view;
  wlr_output *output;
  wlr_output_damage *output_damage;
  wlr_output_layout *output_layout;
};

Output::~Output() {
  wl_list_init(&frame_.link);
  wl_list_remove(&frame_.link);
  wl_list_init(&destroy_.link);
  wl_list_remove(&destroy_.link);
}

Output::Output(Controller *server, struct wlr_output *output, wlr_output_damage *damage, wlr_output_layout *layout)
  : output_(output)
  , server_(server)
  , damage_(damage)
  , layout_(layout)
  { }

void Output::destroy() {
  server_->remove_output(this);
}

static void scissor_output(struct wlr_output *wlr_output, pixman_box32_t *rect) {
  struct wlr_renderer *renderer = wlr_backend_get_renderer(wlr_output->backend);

  struct wlr_box box = {
    .x = rect->x1,
    .y = rect->y1,
    .width = rect->x2 - rect->x1,
    .height = rect->y2 - rect->y1,
  };

  int ow, oh;
  wlr_output_transformed_resolution(wlr_output, &ow, &oh);

  enum wl_output_transform transform = wlr_output_transform_invert(wlr_output->transform);
  wlr_box_transform(&box, &box, transform, ow, oh);

  wlr_renderer_scissor(renderer, &box);
}

static void render_surface(wlr_surface *surface, int sx, int sy, void *data) {
  if (surface == NULL) {
    return;
  }

  /* This function is called for every surface that needs to be rendered. */
  auto rdata = static_cast<struct render_data*>(data);
  View *view = rdata->view;
  wlr_output_layout *layout = rdata->layout;
  struct wlr_output *output = rdata->output;
  pixman_region32_t *buffer_damage = rdata->buffer_damage;

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

  wlr_output_layout_output_coords(layout, output, &ox, &oy);

  ox += view->x + sx;
  oy += view->y + sy;

  /* We also have to apply the scale factor for HiDPI outputs. This is only
   * part of the puzzle, TinyWL does not fully support HiDPI. */
  struct wlr_box box = {
    .x = static_cast<int>(ox) * static_cast<int>(output->scale),
    .y = static_cast<int>(oy) * static_cast<int>(output->scale),
    .width = surface->current.width * static_cast<int>(output->scale),
    .height = surface->current.height * static_cast<int>(output->scale),
  };

  float matrix[9];
  enum wl_output_transform transform = wlr_output_transform_invert(surface->current.transform);
  wlr_matrix_project_box(matrix, &box, transform, 0, output->transform_matrix);

  pixman_region32_t damage;
  pixman_region32_init(&damage);
  pixman_region32_union_rect(&damage, &damage, box.x, box.y, box.width, box.height);
  pixman_region32_intersect(&damage, &damage, buffer_damage);

  int nrects;
  pixman_box32_t *rects = pixman_region32_rectangles(&damage, &nrects);
  for (int i = 0; i < nrects; i++) {
    scissor_output(output, &rects[i]);
    wlr_render_texture_with_matrix(rdata->renderer, texture, matrix, 1);
  }

  pixman_region32_fini(&damage);

  wlr_surface_send_frame_done(surface, rdata->when);
}

void surface_damage_output(wlr_surface *surface, int sx, int sy, void *data) {
  auto damage_data = static_cast<damage_iterator_data*>(data);
  auto view = damage_data->view;
  auto output = damage_data->output;
  auto output_damage = damage_data->output_damage;
  auto layout = damage_data->output_layout;

  double output_x = view->x + sx;
  double output_y = view->y + sy;
  wlr_output_layout_output_coords(layout, output, &output_x, &output_y);

  pixman_region32_t damage;
  pixman_region32_init(&damage);
  wlr_surface_get_effective_damage(surface, &damage);
  pixman_region32_translate(&damage, output_x, output_y);

  wlr_region_scale(&damage, &damage, output->scale);

  wlr_output_damage_add(output_damage, &damage);
  pixman_region32_fini(&damage);
}

void Output::take_whole_damage() {
  wlr_output_damage_add_whole(damage_);
}

void Output::take_damage(const View *view) {
  damage_iterator_data data = {
    .view = view,
    .output = output_,
    .output_damage = damage_,
    .output_layout = layout_
  };
  view->for_each_surface(surface_damage_output, &data);
}

void Output::render() const {
  wlr_renderer *renderer = server_->renderer;

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);

  if (!wlr_output_attach_render(output_, NULL)) {
    return;
  }

  bool needs_frame = false;
  pixman_region32_t buffer_damage;
  pixman_region32_init(&buffer_damage);
  wlr_output_damage_attach_render(damage_, &needs_frame, &buffer_damage);

  if (!needs_frame) {
    return;
  }

  /* The "effective" resolution can change if you rotate your outputs. */
  // int width, height;
  // wlr_output_effective_resolution(wlr_output, &width, &height);

  /* Begin the renderer (calls glViewport and some other GL sanity checks) */
  wlr_renderer_begin(renderer, output_->width, output_->height);

  float color[4] = {0.0, 0.0, 0.0, 1.0};
  // wlr_renderer_clear(renderer, color);

  // Clear everything that has been damaged
  int nrects;
  pixman_box32_t *rects = pixman_region32_rectangles(&buffer_damage, &nrects);
  for (int i = 0; i < nrects; i++) {
    scissor_output(output_, &rects[i]);
    wlr_renderer_clear(renderer, color);
  }

  View *view;
  wl_list_for_each_reverse(view, &server_->views, link) {
    if (!view->mapped) {
      continue;
    }

    struct render_data render_data = {
      .output = output_,
      .renderer = renderer,
      .view = view,
      .when = &now,
      .layout = server_->output_layout,
      .buffer_damage = &buffer_damage
    };

    view->for_each_surface(render_surface, &render_data);
  }

  pixman_region32_fini(&buffer_damage);

  /* Hardware cursors are rendered by the GPU on a separate plane, and can be
   * moved around without re-rendering what's beneath them - which is more
   * efficient. However, not all hardware supports hardware cursors. For this
   * reason, wlroots provides a software fallback, which we ask it to render
   * here. wlr_cursor handles configuring hardware vs software cursors for you,
   * and this function is a no-op when hardware cursors are in use. */
  wlr_output_render_software_cursors(output_, NULL);

  /* Conclude rendering and swap the buffers, showing the final frame
   * on-screen. */
  wlr_renderer_end(renderer);

  pixman_region32_t frame_damage;
  pixman_region32_init(&frame_damage);

  wlr_output_set_damage(output_, &frame_damage);
  wlr_output_commit(output_);

  pixman_region32_fini(&frame_damage);
}
