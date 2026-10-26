#include <strings.h>
#include "sway/config.h"
#include "sway/commands.h"
#include "sway/input/input-manager.h"

struct cmd_results *input_cmd_hires_wheel(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "hires_wheel", EXPECTED_AT_LEAST, 1))) {
		return error;
	}
	struct input_config *ic = config->handler_context.input_config;
	if (!ic) {
		return cmd_results_new(CMD_FAILURE, "No input device defined.");
	}

	if (strcasecmp(argv[0], "auto") == 0) {
		ic->hires_wheel = HIRES_WHEEL_AUTO;
	} else if (strcasecmp(argv[0], "true") == 0) {
		ic->hires_wheel = HIRES_WHEEL_ON;
	} else if (strcasecmp(argv[0], "false") == 0) {
		ic->hires_wheel = HIRES_WHEEL_OFF;
	} else {
		return cmd_results_new(CMD_INVALID,
			"Expected 'hires_wheel true|false|auto'");
	}

	return cmd_results_new(CMD_SUCCESS, NULL);
}
