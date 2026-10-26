#include <stdlib.h>
#include "sway/config.h"
#include "sway/commands.h"
#include "sway/input/input-manager.h"

struct cmd_results *input_cmd_hires_wheel_timeout(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "hires_wheel_timeout", EXPECTED_EQUAL_TO, 1))) {
		return error;
	}
	struct input_config *ic = config->handler_context.input_config;
	if (!ic) {
		return cmd_results_new(CMD_FAILURE, "No input device defined.");
	}

	char *inv;
	long value = strtol(argv[0], &inv, 10);
	if (*inv != '\0' || value < 0 || value > 10000) {
		return cmd_results_new(CMD_INVALID,
			"Expected a timeout in ms between 0 and 10000, 0 to disable");
	}
	ic->hires_wheel_timeout = value;

	return cmd_results_new(CMD_SUCCESS, NULL);
}
