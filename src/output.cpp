#include "output.h"

#include <time.h>
#include <vector>
#include <memory>
#include <iostream>

#include "view.h"
#include "server.h"

extern "C" {
  #include <wlr/backend.h>
  #include <wlr/interfaces/wlr_output.h>
  #include <wlr/render/wlr_renderer.h>
  #include <wlr/types/wlr_compositor.h>
  #include <wlr/types/wlr_output_layout.h>
  #include <wlr/types/wlr_cursor.h>
  #include <wlr/types/wlr_xcursor_manager.h>
  #include <wlr/types/wlr_seat.h>
  #include <wlr/types/wlr_xdg_shell.h>
  #include <wlr/types/wlr_matrix.h>
  #include <wlr/util/log.h>
}

struct render_data {
	const Output *output;
	struct wlr_renderer *renderer;
	struct View *view;
	struct timespec *when;
};


Output::Output(struct wlr_output *output_, const Server *server_)
  : output(output_)
  , server(server_) {
    clock_gettime(CLOCK_MONOTONIC, &last_frame);
  output->data = this;
}

static void render_surface(struct wlr_surface *surface, int sx, int sy, void *data) {
	auto rdata = static_cast<struct render_data *>(data);
	View *view = rdata->view;
  const Server *server = rdata->output->server;
	struct wlr_output *output = rdata->output->output;

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

  wlr_output_layout_output_coords(server->layout, output, &ox, &oy);

  ox += view->x + sx;
  oy += view->y + sy;

	struct wlr_box box = {
		.x = (int)ox * (int)output->scale,
		.y = (int)oy * (int)output->scale,
		.width = surface->current.width * (int)output->scale,
		.height = surface->current.height * (int)output->scale,
	};

	float matrix[9];
	enum wl_output_transform transform = wlr_output_transform_invert(surface->current.transform);
	wlr_matrix_project_box(matrix, &box, transform, 0, output->transform_matrix);
	wlr_render_texture_with_matrix(rdata->renderer, texture, matrix, 1);
	wlr_surface_send_frame_done(surface, rdata->when);
  // std::clog << "rendered" << std::endl;
}

void Output::render(const std::vector<std::shared_ptr<View>>& views) const {
  struct wlr_renderer *renderer = wlr_backend_get_renderer(output->backend);

  wlr_output_attach_render(output, NULL);
  wlr_renderer_begin(renderer, output->width, output->height);

  float color[4] = {1.0, 0, 0, 1.0};
  wlr_renderer_clear(renderer, color);

  struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

  // std::clog << views.size() << std::endl;
  for (auto& view : views) {
    if (!view->mapped()) {
      continue;
    }

    struct render_data rdata = {
			.output = this,
			.renderer = renderer,
			.view = view.get(),
			.when = &now
		};

    wlr_xdg_surface_for_each_surface(view->xdg_surface_, render_surface, &rdata);
  }

  wlr_output_render_software_cursors(output, NULL);

  wlr_renderer_end(renderer);
  wlr_output_commit(output);
}
