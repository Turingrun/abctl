#include "cli.h"
#include "ab_slot.h"

#include <inttypes.h>
#include <string.h>

const char *cli_parse(int argc, char **argv, CliRequest *request)
{
    *request = (CliRequest){.command = CLI_HELP};
    int i = 1;
    if (i == argc)
        return "a command is required";
    const char *command = argv[i++];
    if (strcmp(command, "show") == 0)
        request->command = CLI_SHOW;
    else if (strcmp(command, "init") == 0)
        request->command = CLI_INIT;
    else if (strcmp(command, "--help") == 0)
        request->command = CLI_HELP;
    else if (strcmp(command, "set-active") == 0)
        request->command = CLI_ACTIVE;
    else if (strcmp(command, "mark-successful") == 0)
        request->command = CLI_SUCCESSFUL;
    else if (strcmp(command, "mark-unbootable") == 0)
        request->command = CLI_UNBOOTABLE;
    else
        return "unknown command";
    if (request->command == CLI_SHOW || request->command == CLI_INIT ||
        request->command == CLI_HELP)
        return i == argc ? NULL : "unexpected argument";
    if (i == argc)
        return "a slot (a, b, 0, or 1) is required";
    const char *slot = argv[i++];
    if (strcmp(slot, "a") == 0 || strcmp(slot, "0") == 0)
        request->slot = 0;
    else if (strcmp(slot, "b") == 0 || strcmp(slot, "1") == 0)
        request->slot = 1;
    else
        return "invalid slot: use a, b, 0, or 1";
    return i == argc ? NULL : "unexpected argument";
}

void cli_help(FILE *out)
{
    fputs("Usage: abctl COMMAND [ARGS]\n"
          "Commands:\n"
          "  show\n"
          "  set-active SLOT\n"
          "  mark-successful SLOT\n"
          "  mark-unbootable SLOT\n"
          "  init                  Reset metadata, including reserved bytes.\n"
          "  --help\n", out);
}

void cli_validation_errors(const AbValidation *validation)
{
    if (!validation->magic_ok)
        fputs("abctl: invalid metadata magic\n", stderr);
    if (!validation->version_ok)
        fputs("abctl: unsupported metadata major version\n", stderr);
    if (!validation->crc_ok)
        fputs("abctl: metadata CRC32 mismatch\n", stderr);
    if (!validation->fields_ok)
        fputs("abctl: slot fields out of range\n", stderr);
}

void cli_show(const char *path, const AvbABData *data, const AbValidation *validation)
{
    bool valid = ab_metadata_valid(validation);
    printf("target: %s\noffset: 2048\nsize: 32\n", path);
    printf("magic: %02x %02x %02x %02x (%s)\n",
           data->magic[0], data->magic[1], data->magic[2], data->magic[3],
           validation->magic_ok ? "ok" : "invalid");
    printf("version: %u.%u (%s)\n", data->version_major, data->version_minor,
           validation->version_ok ? "supported" : "unsupported");
    for (unsigned i = 0; i < 2; ++i) {
        const AvbABSlotData *slot = &data->slots[i];
        printf("slot %c: priority=%u tries_remaining=%u successful_boot=%u bootable=%s\n",
               'a' + (int)i, slot->priority, slot->tries_remaining, slot->successful_boot,
               !valid ? "unknown" : (ab_slot_bootable(slot) ? "yes" : "no"));
        if (valid && !ab_slot_canonical(slot))
            fprintf(stderr, "abctl: warning: slot %c is noncanonical; standard slot actions "
                    "normalize it to 0/0/0 before applying changes\n", 'a' + (int)i);
    }
    printf("last_boot: %u (stored value, not the running Linux slot)\n", data->last_boot);
    printf("crc32_stored: 0x%08" PRIx32 "\ncrc32_computed: 0x%08" PRIx32 "\n",
           validation->stored_crc, validation->computed_crc);
    printf("validation: %s\n", valid ? "ok" : "failed");
    int preferred = valid ? ab_slot_preferred(data) : -1;
    printf("preferred_slot: %s (metadata selection only)\n",
           !valid ? "unknown" : (preferred == 0 ? "a" : preferred == 1 ? "b" : "none"));
}
