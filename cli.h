#ifndef ABCTL_CLI_H
#define ABCTL_CLI_H

#include "ab_metadata.h"
#include "ab_slot.h"
#include <stdio.h>

typedef enum {
    CLI_SHOW, CLI_ACTIVE, CLI_SUCCESSFUL, CLI_UNBOOTABLE,
    CLI_SET_SLOT, CLI_INIT, CLI_HELP
} CliCommand;

typedef struct {
    const char *path;
    CliCommand command;
    unsigned slot;
    AbSlotUpdate update;
} CliRequest;

/* NULL on success, otherwise a static error description. */
const char *cli_parse(int argc, char **argv, CliRequest *request);
void cli_help(FILE *out);
void cli_show(const char *path, const AvbABData *data, const AbValidation *validation);
void cli_validation_errors(const AbValidation *validation);

#endif
