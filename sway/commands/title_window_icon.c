#include <strings.h>
#include "sway/commands.h"
#include "sway/config.h"
#include "sway/view_icon.h"

struct cmd_results *cmd_title_window_icon(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "title_window_icon", EXPECTED_AT_LEAST, 1))) {
		return error;
	}

	if (strcmp(argv[0], "padding") == 0) {
		if ((error = checkarg(argc, "title_window_icon padding", EXPECTED_EQUAL_TO, 2))) {
			return error;
		}
		config->title_window_icon_padding = atoi(argv[1]);
		config->title_window_icon = true;
	} else if (strcasecmp(argv[0], "on") == 0 || strcmp(argv[0], "yes") == 0) {
		config->title_window_icon = true;
	} else if (strcasecmp(argv[0], "off") == 0 || strcmp(argv[0], "no") == 0) {
		config->title_window_icon = false;
	} else if (strcmp(argv[0], "toggle") == 0) {
		config->title_window_icon = !config->title_window_icon;
	} else {
		return cmd_results_new(CMD_INVALID,
			"Expected 'title_window_icon on|off|toggle|padding <px>'");
	}

	view_icon_update_all();

	return cmd_results_new(CMD_SUCCESS, NULL);
}
