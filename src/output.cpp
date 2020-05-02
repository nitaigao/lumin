#include "output.h"

#include <spdlog/spdlog.h>
#include <wlroots.h>

#include <iostream>
#include <sstream>

#include "cursor.h"
#include "view.h"
#include "server.h"

namespace lumin {

const int ENTER_FRAME_REPEAT_COUNT = 5;

struct render_data {
  wlr_output *output;
  wlr_renderer *renderer;
  View *view;
  timespec *when;
  wlr_output_layout *layout;
  pixman_region32_t *output_damage;
};

struct damage_iterator_data {
  const View *view;
  wlr_output *output;
  wlr_output_damage *output_damage;
  wlr_output_layout *output_layout;
};

Output::~Output()
{
  wl_list_remove(&frame_.link);
  wl_list_remove(&destroy_.link);
}

Output::Output(
  struct wlr_output *output,
  wlr_renderer *renderer,
  wlr_output_damage *damage,
  wlr_output_layout *layout)
  : wlr_output(output)
  , renderer_(renderer)
  , damage_(damage)
  , layout_(layout)
  , enabled_(false)
  , connected_(true)
  , primary_(false)
  , software_cursors_(false)
  , enter_frames_left_(0)
  , top_margin_(0)
{
  destroy_.notify = Output::output_destroy_notify;
  wl_signal_add(&wlr_output->events.destroy, &destroy_);

  frame_.notify = Output::output_frame_notify;
  wl_signal_add(&damage->events.frame, &frame_);
}

void Output::init()
{
  wlr_output_create_global(wlr_output);
}

bool Output::connected() const
{
  return connected_;
}

void Output::set_connected(bool connected)
{
  connected_ = connected;
}

bool Output::primary() const
{
  return primary_;
}

void Output::set_primary(bool primary)
{
  primary_ = primary;
}

void Output::set_mode()
{
  if (wl_list_empty(&wlr_output->modes)) {
    return;
  }

  struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
  if (wlr_output->current_mode == mode) {
    return;
  }

  spdlog::debug("{} mode:{}x{}@{}", id(), mode->width, mode->height, mode->refresh * 0.001);
  wlr_output_set_mode(wlr_output, mode);
}

void Output::send_enter(const std::vector<std::shared_ptr<View>>& views)
{
  if (enter_frames_left_-- > 0) {
    for (auto &view : views) {
      view->enter(this);
    }
  }
}

void Output::set_menubar(View *view)
{
  view->move(x(), y());
  view->resize(wlr_output->width, View::MENU_HEIGHT);
  top_margin_ = View::MENU_HEIGHT;
}

void Output::add_view(View *view)
{
  wlr_box geometry;
  view->geometry(&geometry);

  int inside_x = ((wlr_output->width  / wlr_output->scale) - geometry.width) / 2.0;
  int inside_y = ((wlr_output->height / wlr_output->scale) - geometry.height) / 2.0;

  view->x = inside_x;
  view->y = inside_y;

  if (view->y + geometry.y < top_margin_) {
    view->y = top_margin_ - geometry.y;
  }

  if (view->y + geometry.height > wlr_output->height - view->y) {
    view->resize(geometry.width, wlr_output->height - view->y);
  }
}

void Output::place_cursor(Cursor *cursor)
{
  int cursor_x = x() + (wlr_output->width * 0.5f) / wlr_output->scale;
  int cursor_y = y() + (wlr_output->height * 0.5f) / wlr_output->scale;

  cursor->warp(cursor_x, cursor_y);
}

void Output::maximize_view(View *view)
{
  int new_width = wlr_output->width / wlr_output->scale;
  int new_height = wlr_output->height / wlr_output->scale;

  view->resize(new_width, new_height - top_margin_);

  int new_x = x();
  int new_y = y() + top_margin_;

  view->move(new_x, new_y);
}

void Output::move_view(View *view, double x, double y)
{
  wlr_box box;
  view->geometry(&box);

  if (y + box.y < top_margin_) {
    y = top_margin_ - box.y;
  }

  view->move(x, y);
}

void Output::commit()
{
  if (!wlr_output_commit(wlr_output)) {
    std::cerr << "Failed to commit output" << std::endl;
  }
}

void Output::destroy()
{
  wlr_output_destroy_global(wlr_output);
}

std::string Output::id() const
{
  std::stringstream id;
  id << wlr_output->make << " " << wlr_output->model;
  return id.str();
}

int Output::width() const
{
  return wlr_output->width;
}

int Output::height() const
{
  return wlr_output->height;
}

static void scissor_output(struct wlr_output *wlr_output, pixman_box32_t *rect)
{
  struct wlr_renderer *renderer = wlr_backend_get_renderer(wlr_output->backend);
  assert(renderer);

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
  wlr_renderer *renderer = rdata->renderer;
  pixman_region32_t *output_damage = rdata->output_damage;

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
  pixman_region32_intersect(&damage, &damage, output_damage);

  int nrects;
  pixman_box32_t *rects = pixman_region32_rectangles(&damage, &nrects);
  for (int i = 0; i < nrects; ++i) {
    scissor_output(output, &rects[i]);
    wlr_render_texture_with_matrix(renderer, texture, matrix, 1);
  }

  pixman_region32_fini(&damage);
}

void surface_damage_output(wlr_surface *surface, int sx, int sy, void *data)
{
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

bool Output::is_named(const std::string& name) const
{
  bool match = name.compare(wlr_output->name) == 0;
  return match;
}

void Output::take_whole_damage()
{
  wlr_output_damage_add_whole(damage_);
}

void Output::take_damage(const View *view)
{
  damage_iterator_data data = {
    .view = view,
    .output = wlr_output,
    .output_damage = damage_,
    .output_layout = layout_
  };
  view->for_each_surface(surface_damage_output, &data);
}

void Output::set_enabled(bool enabled)
{
  enter_frames_left_ = ENTER_FRAME_REPEAT_COUNT;

  if (enabled == enabled_) {
    return;
  }

  if (!enabled) {
    wlr_output_rollback(wlr_output);
  }

  wlr_output_enable(wlr_output, enabled);
  enabled_ = enabled;
}

static void send_frame_done(wlr_surface *surface, int sx, int sy, void *data) {
  struct timespec *when = static_cast<struct timespec *>(data);
  wlr_surface_send_frame_done(surface, when);
}

void Output::render(const std::vector<std::shared_ptr<View>>& views) const
{
  if (!enabled_) {
    return;
  }

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);

  bool needs_frame = false;
  pixman_region32_t buffer_damage;
  pixman_region32_init(&buffer_damage);
  wlr_output_damage_attach_render(damage_, &needs_frame, &buffer_damage);

  if (!needs_frame) {
    return;
  }

  std::vector<std::shared_ptr<View>> mapped_views;
  std::copy_if(views.begin(), views.end(), std::back_inserter(mapped_views), [](auto &view) {
    return view->mapped;
  });

  std::vector<std::shared_ptr<View>> active_views;
  std::copy_if(mapped_views.begin(), mapped_views.end(), std::back_inserter(active_views), [](auto &view) {
    return !view->minimized;
  });

  std::vector<std::shared_ptr<View>> top_layer_views;
  bool maximized_view = false;
  std::copy_if(active_views.begin(), active_views.end(), std::back_inserter(top_layer_views), [maximized_view](auto &view) mutable {
    // Cull anything behind a maximized window
    if (view->layer() == VIEW_LAYER_TOP && maximized_view) {
      return false;
    }

    if (view->layer() == VIEW_LAYER_TOP && !maximized_view && view->maximized()) {
      maximized_view = true;
    }

    return view->layer() == VIEW_LAYER_TOP;
  });

  std::vector<std::shared_ptr<View>> overlay_layer_views;
  std::copy_if(active_views.begin(), active_views.end(), std::back_inserter(overlay_layer_views), [](auto &view) mutable {
    return view->layer() == VIEW_LAYER_OVERLAY;
  });

  std::vector<std::shared_ptr<View>> render_list;
  render_list.insert(render_list.end(), overlay_layer_views.begin(), overlay_layer_views.end());
  render_list.insert(render_list.end(), top_layer_views.begin(), top_layer_views.end());

  if (!wlr_output_attach_render(wlr_output, NULL)) {
    return;
  }

  wlr_renderer_begin(renderer_, wlr_output->width, wlr_output->height);

  float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

  int nrects;
  pixman_box32_t *rects = pixman_region32_rectangles(&buffer_damage, &nrects);
  for (int i = 0; i < nrects; ++i) {
    scissor_output(wlr_output, &rects[i]);
    wlr_renderer_clear(renderer_, clear_color);
  }

  for (auto it = render_list.rbegin(); it != render_list.rend(); ++it) {
    auto &view = (*it);

    struct render_data render_data = {
      .output = wlr_output,
      .renderer = renderer_,
      .view = view.get(),
      .when = &now,
      .layout = layout_,
      .output_damage = &buffer_damage,
    };

    view->for_each_surface(render_surface, &render_data);
  }

  wlr_renderer_scissor(renderer_, NULL);
  wlr_output_render_software_cursors(wlr_output, &buffer_damage);
  wlr_renderer_end(renderer_);

  pixman_region32_t frame_damage;
  pixman_region32_init(&frame_damage);

  enum wl_output_transform transform = wlr_output_transform_invert(wlr_output->transform);
  wlr_region_transform(&frame_damage, &damage_->current, transform, wlr_output->width, wlr_output->height);

  wlr_output_set_damage(wlr_output, &frame_damage);
  pixman_region32_fini(&frame_damage);

  wlr_output_commit(wlr_output);

  pixman_region32_fini(&buffer_damage);

  for (auto &view : render_list) {
    view->for_each_surface(send_frame_done, &now);
  }
}

void Output::output_frame_notify(wl_listener *listener, void *data)
{
  Output *output = wl_container_of(listener, output, frame_);
  output->on_frame.emit(output);
}

void Output::output_destroy_notify(wl_listener *listener, void *data)
{
  Output *output = wl_container_of(listener, output, destroy_);
  output->on_destroy.emit(output);
}

int Output::x() const {
  auto box = wlr_output_layout_get_box(layout_, wlr_output);
  return box->x;
}

int Output::y() const {
  auto box = wlr_output_layout_get_box(layout_, wlr_output);
  return box->y;
}

void Output::set_scale(int scale)
{
  wlr_output_set_scale(wlr_output, scale);
}

void Output::set_position(int x, int y)
{
  wlr_output_layout_move(layout_, wlr_output, x, y);
}

void Output::remove_layout()
{
  wlr_output_layout_remove(layout_, wlr_output);
}

void Output::add_layout(int x, int y)
{
  wlr_output_layout_add(layout_, wlr_output, x, y);
}

void Output::lock_software_cursors()
{
  if (software_cursors_) {
    return;
  }
  wlr_output_lock_software_cursors(wlr_output, true);
  software_cursors_ = true;
}

void Output::unlock_software_cursors()
{
  if (!software_cursors_) {
    return;
  }
  wlr_output_lock_software_cursors(wlr_output, false);
  software_cursors_ = false;
}

}  // namespace lumin
