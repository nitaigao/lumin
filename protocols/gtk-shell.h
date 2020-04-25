/* Generated by wayland-scanner 1.18.0 */

#ifndef GTK_SERVER_PROTOCOL_H
#define GTK_SERVER_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include "wayland-server.h"

#ifdef  __cplusplus
extern "C" {
#endif

struct wl_client;
struct wl_resource;

/**
 * @page page_gtk The gtk protocol
 * @section page_ifaces_gtk Interfaces
 * - @subpage page_iface_gtk_shell1 - gtk specific extensions
 * - @subpage page_iface_gtk_surface1 - 
 */
struct gtk_shell1;
struct gtk_surface1;
struct wl_surface;

/**
 * @page page_iface_gtk_shell1 gtk_shell1
 * @section page_iface_gtk_shell1_desc Description
 *
 * gtk_shell is a protocol extension providing additional features for
 * clients implementing it.
 * @section page_iface_gtk_shell1_api API
 * See @ref iface_gtk_shell1.
 */
/**
 * @defgroup iface_gtk_shell1 The gtk_shell1 interface
 *
 * gtk_shell is a protocol extension providing additional features for
 * clients implementing it.
 */
extern const struct wl_interface gtk_shell1_interface;
/**
 * @page page_iface_gtk_surface1 gtk_surface1
 * @section page_iface_gtk_surface1_api API
 * See @ref iface_gtk_surface1.
 */
/**
 * @defgroup iface_gtk_surface1 The gtk_surface1 interface
 */
extern const struct wl_interface gtk_surface1_interface;

#ifndef GTK_SHELL1_CAPABILITY_ENUM
#define GTK_SHELL1_CAPABILITY_ENUM
enum gtk_shell1_capability {
	GTK_SHELL1_CAPABILITY_GLOBAL_APP_MENU = 1,
	GTK_SHELL1_CAPABILITY_GLOBAL_MENU_BAR = 2,
	GTK_SHELL1_CAPABILITY_DESKTOP_ICONS = 3,
};
#endif /* GTK_SHELL1_CAPABILITY_ENUM */

/**
 * @ingroup iface_gtk_shell1
 * @struct gtk_shell1_interface
 */
struct gtk_shell1_interface {
	/**
	 */
	void (*get_gtk_surface)(struct wl_client *client,
				struct wl_resource *resource,
				uint32_t gtk_surface,
				struct wl_resource *surface);
	/**
	 */
	void (*set_startup_id)(struct wl_client *client,
			       struct wl_resource *resource,
			       const char *startup_id);
	/**
	 */
	void (*system_bell)(struct wl_client *client,
			    struct wl_resource *resource,
			    struct wl_resource *surface);
	/**
	 * @since 3
	 */
	void (*notify_launch)(struct wl_client *client,
			      struct wl_resource *resource,
			      const char *startup_id);
};

#define GTK_SHELL1_CAPABILITIES 0

/**
 * @ingroup iface_gtk_shell1
 */
#define GTK_SHELL1_CAPABILITIES_SINCE_VERSION 1

/**
 * @ingroup iface_gtk_shell1
 */
#define GTK_SHELL1_GET_GTK_SURFACE_SINCE_VERSION 1
/**
 * @ingroup iface_gtk_shell1
 */
#define GTK_SHELL1_SET_STARTUP_ID_SINCE_VERSION 1
/**
 * @ingroup iface_gtk_shell1
 */
#define GTK_SHELL1_SYSTEM_BELL_SINCE_VERSION 1
/**
 * @ingroup iface_gtk_shell1
 */
#define GTK_SHELL1_NOTIFY_LAUNCH_SINCE_VERSION 3

/**
 * @ingroup iface_gtk_shell1
 * Sends an capabilities event to the client owning the resource.
 * @param resource_ The client's resource
 */
static inline void
gtk_shell1_send_capabilities(struct wl_resource *resource_, uint32_t capabilities)
{
	wl_resource_post_event(resource_, GTK_SHELL1_CAPABILITIES, capabilities);
}

#ifndef GTK_SURFACE1_STATE_ENUM
#define GTK_SURFACE1_STATE_ENUM
enum gtk_surface1_state {
	GTK_SURFACE1_STATE_TILED = 1,
	/**
	 * @since 2
	 */
	GTK_SURFACE1_STATE_TILED_TOP = 2,
	/**
	 * @since 2
	 */
	GTK_SURFACE1_STATE_TILED_RIGHT = 3,
	/**
	 * @since 2
	 */
	GTK_SURFACE1_STATE_TILED_BOTTOM = 4,
	/**
	 * @since 2
	 */
	GTK_SURFACE1_STATE_TILED_LEFT = 5,
};
/**
 * @ingroup iface_gtk_surface1
 */
#define GTK_SURFACE1_STATE_TILED_TOP_SINCE_VERSION 2
/**
 * @ingroup iface_gtk_surface1
 */
#define GTK_SURFACE1_STATE_TILED_RIGHT_SINCE_VERSION 2
/**
 * @ingroup iface_gtk_surface1
 */
#define GTK_SURFACE1_STATE_TILED_BOTTOM_SINCE_VERSION 2
/**
 * @ingroup iface_gtk_surface1
 */
#define GTK_SURFACE1_STATE_TILED_LEFT_SINCE_VERSION 2
#endif /* GTK_SURFACE1_STATE_ENUM */

#ifndef GTK_SURFACE1_EDGE_CONSTRAINT_ENUM
#define GTK_SURFACE1_EDGE_CONSTRAINT_ENUM
enum gtk_surface1_edge_constraint {
	GTK_SURFACE1_EDGE_CONSTRAINT_RESIZABLE_TOP = 1,
	GTK_SURFACE1_EDGE_CONSTRAINT_RESIZABLE_RIGHT = 2,
	GTK_SURFACE1_EDGE_CONSTRAINT_RESIZABLE_BOTTOM = 3,
	GTK_SURFACE1_EDGE_CONSTRAINT_RESIZABLE_LEFT = 4,
};
#endif /* GTK_SURFACE1_EDGE_CONSTRAINT_ENUM */

/**
 * @ingroup iface_gtk_surface1
 * @struct gtk_surface1_interface
 */
struct gtk_surface1_interface {
	/**
	 */
	void (*set_dbus_properties)(struct wl_client *client,
				    struct wl_resource *resource,
				    const char *application_id,
				    const char *app_menu_path,
				    const char *menubar_path,
				    const char *window_object_path,
				    const char *application_object_path,
				    const char *unique_bus_name);
	/**
	 */
	void (*set_modal)(struct wl_client *client,
			  struct wl_resource *resource);
	/**
	 */
	void (*unset_modal)(struct wl_client *client,
			    struct wl_resource *resource);
	/**
	 */
	void (*present)(struct wl_client *client,
			struct wl_resource *resource,
			uint32_t time);
	/**
	 * @since 3
	 */
	void (*request_focus)(struct wl_client *client,
			      struct wl_resource *resource,
			      const char *startup_id);
};

#define GTK_SURFACE1_CONFIGURE 0
#define GTK_SURFACE1_CONFIGURE_EDGES 1

/**
 * @ingroup iface_gtk_surface1
 */
#define GTK_SURFACE1_CONFIGURE_SINCE_VERSION 1
/**
 * @ingroup iface_gtk_surface1
 */
#define GTK_SURFACE1_CONFIGURE_EDGES_SINCE_VERSION 2

/**
 * @ingroup iface_gtk_surface1
 */
#define GTK_SURFACE1_SET_DBUS_PROPERTIES_SINCE_VERSION 1
/**
 * @ingroup iface_gtk_surface1
 */
#define GTK_SURFACE1_SET_MODAL_SINCE_VERSION 1
/**
 * @ingroup iface_gtk_surface1
 */
#define GTK_SURFACE1_UNSET_MODAL_SINCE_VERSION 1
/**
 * @ingroup iface_gtk_surface1
 */
#define GTK_SURFACE1_PRESENT_SINCE_VERSION 1
/**
 * @ingroup iface_gtk_surface1
 */
#define GTK_SURFACE1_REQUEST_FOCUS_SINCE_VERSION 3

/**
 * @ingroup iface_gtk_surface1
 * Sends an configure event to the client owning the resource.
 * @param resource_ The client's resource
 */
static inline void
gtk_surface1_send_configure(struct wl_resource *resource_, struct wl_array *states)
{
	wl_resource_post_event(resource_, GTK_SURFACE1_CONFIGURE, states);
}

/**
 * @ingroup iface_gtk_surface1
 * Sends an configure_edges event to the client owning the resource.
 * @param resource_ The client's resource
 */
static inline void
gtk_surface1_send_configure_edges(struct wl_resource *resource_, struct wl_array *constraints)
{
	wl_resource_post_event(resource_, GTK_SURFACE1_CONFIGURE_EDGES, constraints);
}

#ifdef  __cplusplus
}
#endif

#endif
