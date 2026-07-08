#include <cairo.h>
#include <drm_fourcc.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/render/color.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_toplevel_icon_v1.h>
#include <wlr/util/log.h>
#include "types/wlr_buffer.h"
#include "config.h"
#include "list.h"
#include "log.h"
#include "sway/sway_text_node.h"
#include "sway/tree/container.h"
#include "sway/tree/root.h"
#include "sway/tree/view.h"
#include "sway/view_icon.h"
#include <wlr/types/wlr_xdg_shell.h>
#include "swaybar/tray/icon.h"

#if HAVE_GDK_PIXBUF
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif

int view_icon_preferred_size(void) {
	int height = container_titlebar_height();
	return height > 4 ? height - 4 : height;
}

void view_icon_update_manager_sizes(struct wlr_xdg_toplevel_icon_manager_v1 *manager) {
	if (!manager) {
		return;
	}
	int size = view_icon_preferred_size();
	wlr_xdg_toplevel_icon_manager_v1_set_sizes(manager, &size, 1);
}

static void view_icon_destroy_owned(struct sway_view *view);

static void view_icon_clear_scene(struct sway_container *con) {
	if (!con || !con->title_bar.icon) {
		return;
	}
	wlr_scene_buffer_set_buffer(con->title_bar.icon, NULL);
	wlr_scene_node_set_enabled(&con->title_bar.icon->node, false);
	con->title_bar.icon_width = 0;
	con->title_bar.icon_height = 0;
	container_arrange_title_bar(con);
}

static void view_icon_clear(struct sway_view *view) {
	if (!view->container) {
		return;
	}
	view_icon_clear_scene(view->container);
	view_icon_destroy_owned(view);
}

static void view_icon_destroy_owned(struct sway_view *view) {
	if (!view->owned_icon_buffer) {
		return;
	}
	if (!view->owned_icon_buffer->dropped) {
		wlr_buffer_drop(view->owned_icon_buffer);
	}
	view->owned_icon_buffer = NULL;
}

void view_icon_release(struct sway_view *view) {
	if (view->xdg_icon) {
		wlr_xdg_toplevel_icon_v1_unref(view->xdg_icon);
		view->xdg_icon = NULL;
	}
	view_icon_clear(view);
}

static struct wlr_scene_buffer *view_icon_ensure_scene(struct sway_container *con) {
	if (con->title_bar.icon) {
		return con->title_bar.icon;
	}
	con->title_bar.icon = wlr_scene_buffer_create(con->title_bar.tree, NULL);
	if (!con->title_bar.icon) {
		return NULL;
	}
	if (con->title_bar.title_text) {
		wlr_scene_node_raise_to_top(con->title_bar.title_text->node);
	}
	if (con->title_bar.marks_text) {
		wlr_scene_node_raise_to_top(con->title_bar.marks_text->node);
	}
	return con->title_bar.icon;
}

static uint8_t view_icon_premul_channel(uint8_t color, uint8_t alpha) {
	uint32_t z = (uint32_t)color * alpha + 0x80;
	return (z + (z >> 8)) >> 8;
}

static void view_icon_write_pixel_bgra(uint8_t *dst, uint8_t r, uint8_t g,
		uint8_t b, uint8_t a) {
	dst[0] = view_icon_premul_channel(b, a);
	dst[1] = view_icon_premul_channel(g, a);
	dst[2] = view_icon_premul_channel(r, a);
	dst[3] = a;
}

static struct wlr_buffer *view_icon_buffer_from_pixels(uint32_t *pixels,
		uint32_t width, uint32_t height) {
	size_t stride = width * 4;
	size_t size = stride * height;
	uint32_t *copy = malloc(size);
	if (!copy) {
		return NULL;
	}
	memcpy(copy, pixels, size);

	struct wlr_readonly_data_buffer *readonly = readonly_data_buffer_create(
		DRM_FORMAT_ARGB8888, stride, width, height, copy);
	if (!readonly) {
		free(copy);
		return NULL;
	}
	return &readonly->base;
}

static struct wlr_buffer *view_icon_buffer_from_cairo(cairo_surface_t *surface) {
	cairo_surface_flush(surface);
	int width = cairo_image_surface_get_width(surface);
	int height = cairo_image_surface_get_height(surface);
	if (width <= 0 || height <= 0) {
		return NULL;
	}
	return view_icon_buffer_from_pixels(
		(uint32_t *)cairo_image_surface_get_data(surface), width, height);
}

static struct wlr_buffer *view_icon_buffer_from_wlr(struct wlr_buffer *src) {
	void *data = NULL;
	uint32_t format = 0;
	size_t stride = 0;
	if (!wlr_buffer_begin_data_ptr_access(src, WLR_BUFFER_DATA_PTR_ACCESS_READ,
			&data, &format, &stride)) {
		return NULL;
	}

	uint32_t width = src->width;
	uint32_t height = src->height;
	size_t out_stride = width * 4;
	uint8_t *out = calloc(out_stride, height);
	if (!out) {
		wlr_buffer_end_data_ptr_access(src);
		return NULL;
	}

	const uint8_t *src_bytes = data;
	for (uint32_t y = 0; y < height; y++) {
		const uint8_t *row = src_bytes + y * stride;
		uint8_t *out_row = out + y * out_stride;
		for (uint32_t x = 0; x < width; x++) {
			const uint8_t *p = row + x * 4;
			uint8_t *d = out_row + x * 4;
			uint8_t r, g, b, a;
			switch (format) {
			case DRM_FORMAT_ARGB8888:
				/* wl_shm ARGB8888 is B,G,R,A in memory */
				b = p[0];
				g = p[1];
				r = p[2];
				a = p[3];
				d[0] = b;
				d[1] = g;
				d[2] = r;
				d[3] = a;
				break;
			case DRM_FORMAT_ABGR8888:
				/* R,G,B,A in memory */
				r = p[0];
				g = p[1];
				b = p[2];
				a = p[3];
				view_icon_write_pixel_bgra(d, r, g, b, a);
				break;
			case DRM_FORMAT_XRGB8888:
				b = p[0];
				g = p[1];
				r = p[2];
				view_icon_write_pixel_bgra(d, r, g, b, 0xff);
				break;
			default:
				free(out);
				wlr_buffer_end_data_ptr_access(src);
				return NULL;
			}
		}
	}
	wlr_buffer_end_data_ptr_access(src);

	struct wlr_readonly_data_buffer *readonly = readonly_data_buffer_create(
		DRM_FORMAT_ARGB8888, out_stride, width, height, out);
	if (!readonly) {
		free(out);
		return NULL;
	}
	return &readonly->base;
}

static list_t *icon_themes = NULL;
static list_t *icon_basedirs = NULL;

static void view_icon_init_themes(void) {
	if (icon_themes) {
		return;
	}
	init_themes(&icon_themes, &icon_basedirs);
}

static char *view_icon_gtk_theme(void) {
	const char *env = getenv("GTK_ICON_THEME");
	if (env && *env) {
		return strdup(env);
	}

	const char *home = getenv("HOME");
	if (!home) {
		return NULL;
	}

	char path[PATH_MAX];
	const char *settings[] = {
		"/.config/gtk-3.0/settings.ini",
		"/.config/gtk-4.0/settings.ini",
	};
	for (size_t i = 0; i < sizeof(settings) / sizeof(settings[0]); i++) {
		if (snprintf(path, sizeof(path), "%s%s", home, settings[i]) >= (int)sizeof(path)) {
			continue;
		}
		FILE *file = fopen(path, "r");
		if (!file) {
			continue;
		}
		char line[512];
		while (fgets(line, sizeof(line), file)) {
			if (strncmp(line, "gtk-icon-theme-name=", 20) != 0) {
				continue;
			}
			char *value = line + 20;
			while (*value == ' ' || *value == '\t') {
				value++;
			}
			size_t len = strlen(value);
			while (len > 0 && (value[len - 1] == '\n' || value[len - 1] == '\r')) {
				value[--len] = '\0';
			}
			fclose(file);
			return len > 0 ? strdup(value) : NULL;
		}
		fclose(file);
	}
	return NULL;
}

static char *view_icon_alt_name(const char *name) {
	if (!name || !strchr(name, '_')) {
		return NULL;
	}
	char *alt = strdup(name);
	if (!alt) {
		return NULL;
	}
	for (char *p = alt; *p; p++) {
		if (*p == '_') {
			*p = '-';
		}
	}
	return alt;
}

static char *view_icon_find_path(const char *name, int size) {
	if (!name || !*name) {
		return NULL;
	}
	view_icon_init_themes();

	char *gtk_theme = view_icon_gtk_theme();
	char *alt_name = view_icon_alt_name(name);
	const char *names[] = { name, alt_name, NULL };

	char *path = NULL;
	for (int n = 0; names[n] && !path; n++) {
		int min_size = 0, max_size = 0;
		if (gtk_theme) {
			path = find_icon(icon_themes, icon_basedirs,
				(char *)names[n], size, gtk_theme,
				&min_size, &max_size);
		}
		if (!path) {
			path = find_icon(icon_themes, icon_basedirs,
				(char *)names[n], size, NULL, &min_size, &max_size);
		}
	}

	free(alt_name);
	free(gtk_theme);
	return path;
}

static bool view_icon_try_path(const char *path, struct wlr_buffer **out) {
	if (!path || access(path, R_OK) != 0) {
		return false;
	}

#if HAVE_GDK_PIXBUF
	GError *err = NULL;
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(path, &err);
	if (!pixbuf) {
		if (err) {
			g_error_free(err);
		}
		return false;
	}
	int width = gdk_pixbuf_get_width(pixbuf);
	int height = gdk_pixbuf_get_height(pixbuf);
	int channels = gdk_pixbuf_get_n_channels(pixbuf);
	if (width <= 0 || height <= 0 || channels < 3) {
		g_object_unref(pixbuf);
		return false;
	}
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
		width, height);
	uint8_t *cairo_data = cairo_image_surface_get_data(surface);
	int stride = cairo_image_surface_get_stride(surface);
	const uint8_t *src = gdk_pixbuf_read_pixels(pixbuf);
	int src_stride = gdk_pixbuf_get_rowstride(pixbuf);
	bool has_alpha = gdk_pixbuf_get_has_alpha(pixbuf);
	for (int y = 0; y < height; y++) {
		const uint8_t *row = src + y * src_stride;
		uint8_t *dst = cairo_data + y * stride;
		for (int x = 0; x < width; x++) {
			uint8_t r = row[x * channels + 0];
			uint8_t g = row[x * channels + 1];
			uint8_t b = row[x * channels + 2];
			uint8_t a = has_alpha ? row[x * channels + 3] : 0xff;
			/* cairo ARGB32 is B,G,R,A in memory on little-endian */
			view_icon_write_pixel_bgra(dst + x * 4, r, g, b, a);
		}
	}
	g_object_unref(pixbuf);
	struct wlr_buffer *buffer = view_icon_buffer_from_cairo(surface);
	cairo_surface_destroy(surface);
	if (buffer) {
		*out = buffer;
		return true;
	}
#else
	cairo_surface_t *surface = cairo_image_surface_create_from_png(path);
	if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(surface);
		return false;
	}
	struct wlr_buffer *buffer = view_icon_buffer_from_cairo(surface);
	cairo_surface_destroy(surface);
	if (buffer) {
		*out = buffer;
		return true;
	}
#endif
	return false;
}

static struct wlr_buffer *view_icon_load_named(const char *name) {
	int size = view_icon_preferred_size();
	char *path = view_icon_find_path(name, size);
	if (!path) {
		return NULL;
	}
	struct wlr_buffer *buffer = NULL;
	if (view_icon_try_path(path, &buffer)) {
		free(path);
		return buffer;
	}
	free(path);
	return NULL;
}

static struct wlr_buffer *view_icon_pick_xdg_buffer(struct wlr_xdg_toplevel_icon_v1 *icon,
		int *width, int *height) {
	if (!icon || wl_list_empty(&icon->buffers)) {
		return NULL;
	}
	int target = view_icon_preferred_size();
	struct wlr_xdg_toplevel_icon_v1_buffer *best = NULL;
	int best_diff = INT_MAX;
	struct wlr_xdg_toplevel_icon_v1_buffer *entry;
	wl_list_for_each(entry, &icon->buffers, link) {
		int size = entry->buffer->width;
		int diff = abs(size - target);
		if (diff < best_diff) {
			best_diff = diff;
			best = entry;
		}
	}
	if (!best) {
		return NULL;
	}
	struct wlr_buffer *locked = wlr_buffer_lock(best->buffer);
	if (!locked) {
		return NULL;
	}
	struct wlr_buffer *normalized = view_icon_buffer_from_wlr(locked);
	wlr_buffer_unlock(locked);
	if (!normalized) {
		return NULL;
	}
	*width = normalized->width;
	*height = normalized->height;
	return normalized;
}

#if WLR_HAS_XWAYLAND
#include <wlr/xwayland.h>
#include <xcb/xcb_ewmh.h>

static struct wlr_buffer *view_icon_buffer_from_xwayland(
		struct wlr_xwayland_surface *xsurface, int *width, int *height) {
	xcb_ewmh_get_wm_icon_reply_t icon_reply;
	if (!wlr_xwayland_surface_fetch_icon(xsurface, &icon_reply)) {
		return NULL;
	}

	int target = view_icon_preferred_size();
	uint32_t *data = NULL;
	uint32_t best_w = 0, best_h = 0;
	uint64_t best_len = 0;

	xcb_ewmh_wm_icon_iterator_t iter = xcb_ewmh_get_wm_icon_iterator(&icon_reply);
	for (; iter.rem; xcb_ewmh_get_wm_icon_next(&iter)) {
		uint32_t cur_w = iter.width;
		uint32_t cur_h = iter.height;
		uint64_t cur_len = (uint64_t)cur_w * cur_h;
		if (cur_w == 0 || cur_h == 0) {
			continue;
		}
		bool at_least = cur_w >= (uint32_t)target && cur_h >= (uint32_t)target;
		bool smaller = cur_w < best_w || cur_h < best_h;
		bool larger = cur_w > best_w || cur_h > best_h;
		bool not_preferred = best_w < (uint32_t)target || best_h < (uint32_t)target;
		if (best_len == 0 ||
				(at_least && (smaller || not_preferred)) ||
				(!at_least && not_preferred && larger)) {
			best_len = cur_len;
			best_w = cur_w;
			best_h = cur_h;
			data = iter.data;
		}
		if (best_w == (uint32_t)target && best_h == (uint32_t)target) {
			break;
		}
	}

	if (!data || best_len == 0) {
		xcb_ewmh_get_wm_icon_reply_wipe(&icon_reply);
		return NULL;
	}

	uint32_t *pixels = malloc(best_len * sizeof(uint32_t));
	if (!pixels) {
		xcb_ewmh_get_wm_icon_reply_wipe(&icon_reply);
		return NULL;
	}
	for (uint64_t i = 0; i < best_len; i++) {
		uint32_t pixel = data[i];
		/* _NET_WM_ICON CARD32 is B,G,R,A on little-endian */
		uint8_t *p = (uint8_t *)&pixel;
		view_icon_write_pixel_bgra((uint8_t *)&pixels[i], p[2], p[1], p[0], p[3]);
	}
	xcb_ewmh_get_wm_icon_reply_wipe(&icon_reply);

	*width = best_w;
	*height = best_h;
	return view_icon_buffer_from_pixels(pixels, best_w, best_h);
}
#endif

static void view_icon_apply(struct sway_view *view, struct wlr_buffer *buffer,
		int width, int height, bool owned) {
	if (!view->container || !buffer || width <= 0 || height <= 0) {
		if (owned && buffer) {
			wlr_buffer_drop(buffer);
		}
		return;
	}

	struct sway_container *con = view->container;
	struct wlr_scene_buffer *icon = view_icon_ensure_scene(con);
	if (!icon) {
		if (owned) {
			wlr_buffer_drop(buffer);
		}
		return;
	}

	if (con->title_bar.icon) {
		wlr_scene_buffer_set_buffer(con->title_bar.icon, NULL);
	}
	view_icon_destroy_owned(view);
	if (owned) {
		view->owned_icon_buffer = buffer;
	}

	wlr_scene_buffer_set_buffer(icon, buffer);
	wlr_scene_buffer_set_transfer_function(icon,
		WLR_COLOR_TRANSFER_FUNCTION_SRGB);

	int target = view_icon_preferred_size();
	int dest = MIN(target, MIN(width, height));
	wlr_scene_buffer_set_dest_size(icon, dest, dest);
	wlr_scene_node_set_enabled(&icon->node, true);
	con->title_bar.icon_width = dest;
	con->title_bar.icon_height = dest;
	container_arrange_title_bar(con);
}

void view_icon_set_xdg(struct sway_view *view, struct wlr_xdg_toplevel_icon_v1 *icon) {
	if (view->xdg_icon) {
		wlr_xdg_toplevel_icon_v1_unref(view->xdg_icon);
		view->xdg_icon = NULL;
	}
	if (icon) {
		view->xdg_icon = wlr_xdg_toplevel_icon_v1_ref(icon);
	}
	view_icon_update(view);
}

void xdg_toplevel_icon_manager_v1_handle_set_icon(struct wl_listener *listener,
		void *data) {
	(void)listener;
	const struct wlr_xdg_toplevel_icon_manager_v1_set_icon_event *event = data;
	struct sway_view *view = view_from_wlr_xdg_surface(event->toplevel->base);
	if (!view) {
		return;
	}
	view_icon_set_xdg(view, event->icon);
}

void view_icon_update(struct sway_view *view) {
	if (!config->title_window_icon || !view->container) {
		view_icon_clear(view);
		return;
	}

	struct wlr_buffer *buffer = NULL;
	int width = 0, height = 0;
	bool owned = false;

	if (view->xdg_icon) {
		buffer = view_icon_pick_xdg_buffer(view->xdg_icon, &width, &height);
		if (buffer) {
			owned = true;
		} else if (view->xdg_icon->name) {
			buffer = view_icon_load_named(view->xdg_icon->name);
			owned = buffer != NULL;
			if (buffer) {
				width = buffer->width;
				height = buffer->height;
			}
		}
	}

#if WLR_HAS_XWAYLAND
	if (!buffer && view->type == SWAY_VIEW_XWAYLAND &&
			view->wlr_xwayland_surface) {
		buffer = view_icon_buffer_from_xwayland(view->wlr_xwayland_surface,
			&width, &height);
		owned = buffer != NULL;
	}
#endif

	if (!buffer) {
		const char *name = view_get_app_id(view);
		if (!name || !*name) {
			name = view_get_class(view);
		}
		if (name && *name) {
			buffer = view_icon_load_named(name);
			owned = buffer != NULL;
			if (buffer) {
				width = buffer->width;
				height = buffer->height;
			}
		}
	}

	if (!buffer) {
		view_icon_clear(view);
		return;
	}

	view_icon_apply(view, buffer, width, height, owned);
	if (!owned) {
		wlr_buffer_unlock(buffer);
	}
}

static void view_icon_update_container(struct sway_container *con, void *data) {
	(void)data;
	if (con->view) {
		view_icon_update(con->view);
	}
}

void view_icon_update_all(void) {
	root_for_each_container(view_icon_update_container, NULL);
}
