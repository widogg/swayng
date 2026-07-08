#include <strings.h>
#include "log.h"
#include "sway/commands.h"
#include "sway/config.h"
#include "sway/tree/arrange.h"
#include "sway/tree/container.h"
#include "sway/tree/view.h"
#include "util.h"

// fullscreen [enable|disable|toggle] [global|application]
struct cmd_results *cmd_fullscreen(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "fullscreen", EXPECTED_AT_MOST, 2))) {
		return error;
	}
	if (!root->outputs->length) {
		return cmd_results_new(CMD_FAILURE,
				"Can't run this command while there's no outputs connected.");
	}
	struct sway_container *container = config->handler_context.container;

	if (!container) {
		// If the focus is not a container, do nothing successfully
		return cmd_results_new(CMD_SUCCESS, NULL);
	} else if (!container->pending.workspace) {
		// If in the scratchpad, operate on the highest container
		while (container->pending.parent) {
			container = container->pending.parent;
		}
	}

	bool global = false;
	bool application = false;
	bool toggle = false;
	bool enable = container->pending.fullscreen_mode == FULLSCREEN_NONE;
	bool enable_application = container->pending.fullscreen_application == FULLSCREEN_DISABLED;

	if (argc >= 1) {
		if (strcasecmp(argv[0], "global") == 0) {
			global = true;
		} else if (strcasecmp(argv[0], "application") == 0) {
			application = true;
		} else if (strcasecmp(argv[0], "toggle") == 0) {
			toggle = true;
		} else {
			enable = parse_boolean(argv[0], !enable);
			enable_application = parse_boolean(argv[0], !enable_application);
		}
	}

	if (argc >= 2) {
		if (strcasecmp(argv[1], "global") == 0) {
			global = true;
		} else if (strcasecmp(argv[1], "application") == 0) {
			application = true;
		} else if (strcasecmp(argv[1], "toggle") == 0) {
			toggle = true;
		}
	}

	bool layout_fs = container->pending.fullscreen_mode != FULLSCREEN_NONE;
	bool app_fs = container->pending.fullscreen_application == FULLSCREEN_ENABLED;

	if (application) {
		if (toggle) {
			if (app_fs) {
				container_set_fullscreen_application(container, FULLSCREEN_DISABLED);
				if (layout_fs) {
					container_set_fullscreen(container, FULLSCREEN_NONE);
					container_set_fullscreen_container(container, FULLSCREEN_DISABLED);
					arrange_root();
				}
			} else {
				container_set_fullscreen_application(container, FULLSCREEN_ENABLED);
			}
		} else {
			container_set_fullscreen_application(container,
				enable_application ? FULLSCREEN_ENABLED : FULLSCREEN_DISABLED);
		}
		return cmd_results_new(CMD_SUCCESS, NULL);
	}

	enum sway_fullscreen_mode mode = FULLSCREEN_NONE;
	if (toggle) {
		if (layout_fs) {
			container_set_fullscreen(container, FULLSCREEN_NONE);
			container_set_fullscreen_container(container, FULLSCREEN_DISABLED);
		} else {
			mode = global ? FULLSCREEN_GLOBAL : FULLSCREEN_WORKSPACE;
			container_set_fullscreen(container, mode);
			container_set_fullscreen_container(container, FULLSCREEN_ENABLED);
			// Maximize without telling the client; fake fullscreen stays tiled.
			if (app_fs) {
				container_set_fullscreen_application(container, FULLSCREEN_DISABLED);
			}
		}
		arrange_root();
	} else {
		if (enable) {
			mode = global ? FULLSCREEN_GLOBAL : FULLSCREEN_WORKSPACE;
		}
		container_set_fullscreen(container, mode);
		container_set_fullscreen_container(container, mode != FULLSCREEN_NONE);
		arrange_root();
	}

	return cmd_results_new(CMD_SUCCESS, NULL);
}
