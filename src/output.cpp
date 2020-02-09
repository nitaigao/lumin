#include "output.h"

#include <iostream>
#include <sstream>

#include <wlroots.h>

#include "view.h"
#include "server.h"

namespace lumin {

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
  wl_list_remove(&frame_.link);
  wl_list_remove(&destroy_.link);
}

Output::Output(Server *server,
  struct wlr_output *output,
  wlr_renderer *renderer,
  wlr_output_damage *damage,
  wlr_output_layout *layout)
  : wlr_output(output)
  , renderer_(renderer)
  , server_(server)
  , damage_(damage)
  , layout_(layout)
  , enabled_(false)
{
  frame_.notify = Output::output_frame_notify;
  wl_signal_add(&damage->events.frame, &frame_);

  destroy_.notify = Output::output_destroy_notify;
  wl_signal_add(&wlr_output->events.destroy, &destroy_);
}

void Output::init() {
  wlr_output_create_global(wlr_output);
}

void Output::set_mode() {
  if (!wl_list_empty(&wlr_output->modes)) {
    struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
    wlr_output_set_mode(wlr_output, mode);
  }
}

void Output::commit() {
  if (!wlr_output_commit(wlr_output)) {
    std::cerr << "Failed to commit output" << std::endl;
  }
}

void Output::destroy() {
  wlr_output_layout_remove(layout_, wlr_output);
  wlr_output_destroy_global(wlr_output);
}

std::string Output::id() const {
  std::stringstream id;
  id << wlr_output->make << " " << wlr_output->model;
  return id.str();
}

int Output::width() const {
  return wlr_output->width;
}

int Output::height() const {
  return wlr_output->height;
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

  wlr_render_texture_with_matrix(rdata->renderer, texture, matrix, 1);
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

bool Output::is_named(const std::string& name) const {
  bool match = name.compare(wlr_output->name) == 0;
  return match;
}

void Output::take_whole_damage() {
  wlr_output_damage_add_whole(damage_);
}

void Output::take_damage(const View *view) {
  damage_iterator_data data = {
    .view = view,
    .output = wlr_output,
    .output_damage = damage_,
    .output_layout = layout_
  };
  view->for_each_surface(surface_damage_output, &data);
}

void Output::set_enabled(bool enabled) {
  wlr_output_rollback(wlr_output);
  wlr_output_enable(wlr_output, enabled);
  enabled_ = enabled;
}

void Output::render(const std::vector<std::shared_ptr<View>>& views) const {
  if (!enabled_) {
    return;
  }

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);

  if (!wlr_output_attach_render(wlr_output, NULL)) {
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
  wlr_renderer_begin(renderer_, wlr_output->width, wlr_output->height);

  float color[4] = {0.0, 0.0, 0.0, 1.0};
  wlr_renderer_clear(renderer_, color);

  for (auto it = views.rbegin(); it != views.rend(); ++it) {
    auto &view = (*it);
    if (!view->mapped) {
      continue;
    }

    struct render_data render_data = {
      .output = wlr_output,
      .renderer = renderer_,
      .view = view.get(),
      .when = &now,
      .layout = layout_,
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
  wlr_output_render_software_cursors(wlr_output, NULL);

  /* Conclude rendering and swap the buffers, showing the final frame
   * on-screen. */

  pixman_region32_t frame_damage;
  pixman_region32_init(&frame_damage);

  wlr_output_set_damage(wlr_output, &frame_damage);
  wlr_output_commit(wlr_output);
  wlr_renderer_end(renderer_);

  pixman_region32_fini(&frame_damage);
}

void Output::output_frame_notify(wl_listener *listener, void *data) {
  Output *output = wl_container_of(listener, output, frame_);
  output->server_->render_output(output);
}

void Output::output_destroy_notify(wl_listener *listener, void *data) {
  Output *output = wl_container_of(listener, output, destroy_);
  output->server_->remove_output(output);
}

void Output::set_scale(int scale) {
  wlr_output_set_scale(wlr_output, scale);
}

void Output::set_position(int x, int y) {
  wlr_output_layout_add(layout_, wlr_output, x, y);
}

}  // namespace lumin
