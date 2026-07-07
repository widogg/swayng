#ifndef SWAY_VIEW_ICON_H
#define SWAY_VIEW_ICON_H

#include <wlr/types/wlr_xdg_toplevel_icon_v1.h>

struct sway_view;
struct wlr_xdg_toplevel_icon_manager_v1;

int view_icon_preferred_size(void);

void view_icon_update_manager_sizes(struct wlr_xdg_toplevel_icon_manager_v1 *manager);

void view_icon_set_xdg(struct sway_view *view, struct wlr_xdg_toplevel_icon_v1 *icon);

void view_icon_update(struct sway_view *view);

void view_icon_release(struct sway_view *view);

void view_icon_update_all(void);

void xdg_toplevel_icon_manager_v1_handle_set_icon(struct wl_listener *listener,
		void *data);

#endif
