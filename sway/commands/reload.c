#include <limits.h>
#include <string.h>
#include "sway/commands.h"
#include "sway/config.h"
#include "sway/ipc-server.h"
#include "sway/server.h"
#include "sway/tree/arrange.h"
#include "sway/tree/container.h"
#include "sway/tree/root.h"
#include "sway/tree/view.h"
#include "sway/tree/workspace.h"
#include "sway/view_icon.h"
#include "list.h"
#include "log.h"

static void reload_sync_workspace_gaps(struct sway_workspace *ws, void *data) {
	(void)data;
	ws->gaps_outer = config->gaps_outer;
	ws->gaps_inner = config->gaps_inner;
	struct workspace_config *wsc = workspace_find_config(ws->name);
	if (wsc) {
		if (wsc->gaps_outer.top != INT_MIN) {
			ws->gaps_outer.top = wsc->gaps_outer.top;
		}
		if (wsc->gaps_outer.right != INT_MIN) {
			ws->gaps_outer.right = wsc->gaps_outer.right;
		}
		if (wsc->gaps_outer.bottom != INT_MIN) {
			ws->gaps_outer.bottom = wsc->gaps_outer.bottom;
		}
		if (wsc->gaps_outer.left != INT_MIN) {
			ws->gaps_outer.left = wsc->gaps_outer.left;
		}
		if (wsc->gaps_inner != INT_MIN) {
			ws->gaps_inner = wsc->gaps_inner;
		}
	}
	node_set_dirty(&ws->node);
}

static void reload_sync_container_border(struct sway_container *con, void *data) {
	(void)data;
	if (!con->view) {
		return;
	}
	enum sway_container_border default_border;
	int default_thickness;
	if (container_is_floating(con)) {
		default_border = config->floating_border;
		default_thickness = config->floating_border_thickness;
	} else {
		default_border = config->border;
		default_thickness = config->border_thickness;
	}
	if (con->pending.border != default_border) {
		return;
	}
	con->pending.border_thickness = default_thickness;
	container_update(con);
	view_autoconfigure(con->view);
	node_set_dirty(&con->node);
}

static void title_bar_update_iterator(struct sway_container *con, void *data) {
	container_update_title_bar(con);
}

static void do_reload(void *data) {
	// store bar ids to check against new bars for barconfig_update events
	list_t *bar_ids = create_list();
	for (int i = 0; i < config->bars->length; ++i) {
		struct bar_config *bar = config->bars->items[i];
		list_add(bar_ids, strdup(bar->id));
	}

	const char *path = NULL;
	if (config->user_config_path) {
		path = config->current_config_path;
	}

	if (!load_main_config(path, true, false)) {
		sway_log(SWAY_ERROR, "Error(s) reloading config");
		list_free_items_and_destroy(bar_ids);
		return;
	}

	ipc_event_workspace(NULL, NULL, "reload");

	load_swaybars();

	for (int i = 0; i < config->bars->length; ++i) {
		struct bar_config *bar = config->bars->items[i];
		for (int j = 0; j < bar_ids->length; ++j) {
			if (strcmp(bar->id, bar_ids->items[j]) == 0) {
				ipc_event_barconfig_update(bar);
				break;
			}
		}
	}
	list_free_items_and_destroy(bar_ids);

	root_for_each_workspace(reload_sync_workspace_gaps, NULL);
	root_for_each_container(reload_sync_container_border, NULL);
	root_for_each_container(title_bar_update_iterator, NULL);
	view_icon_update_all();

	arrange_root();
}

struct cmd_results *cmd_reload(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "reload", EXPECTED_EQUAL_TO, 0))) {
		return error;
	}

	const char *path = NULL;
	if (config->user_config_path) {
		path = config->current_config_path;
	}

	if (!load_main_config(path, true, true)) {
		return cmd_results_new(CMD_FAILURE, "Error(s) reloading config.");
	}

	// The reload command frees a lot of stuff, so to avoid use-after-frees
	// we schedule the reload to happen using an idle event.
	wl_event_loop_add_idle(server.wl_event_loop, do_reload, NULL);

	return cmd_results_new(CMD_SUCCESS, NULL);
}
