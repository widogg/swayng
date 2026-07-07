#include <string.h>
#include "sway/commands.h"
#include "sway/config.h"
#include "sway/tree/arrange.h"
#include "sway/tree/container.h"
#include "log.h"

struct cmd_results *cmd_titlebar_border_radius(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "titlebar_border_radius", EXPECTED_EQUAL_TO, 1))) {
		return error;
	}

	char *inv;
	int value = strtol(argv[0], &inv, 10);
	if (*inv != '\0' || value < 0 || value > 512) {
		return cmd_results_new(CMD_FAILURE, "Invalid size specified");
	}

	config->decoration.border_radius = value;
	decoration_sync_from_config();

	return cmd_results_new(CMD_SUCCESS, NULL);
}
