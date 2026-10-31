#include "cli.h"

#include <inttypes.h>
#include <string.h>

static bool parse_number(const char *text, unsigned max, uint8_t *result)
{
    if (*text == '\0')
        return false;
    unsigned value = 0;
    for (const char *p = text; *p; ++p) {
        if (*p < '0' || *p > '9')
            return false;
        value = value * 10 + (unsigned)(*p - '0');
        if (value > max)
            return false;
    }
    *result = (uint8_t)value;
    return true;
}

const char *cli_parse(int argc, char **argv, CliRequest *request)
{
    *request = (CliRequest){.path = "misc.img", .command = CLI_HELP};
    int i = 1;
    if (i < argc && strcmp(argv[i], "--misc") == 0) {
        if (++i == argc || argv[i][0] == '\0')
            return "--misc requires a nonempty path";
        request->path = argv[i++];
    }
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
    else if (strcmp(command, "set-slot") == 0)
        request->command = CLI_SET_SLOT;
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
    if (request->command != CLI_SET_SLOT)
        return i == argc ? NULL : "unexpected argument";
    if (i == argc)
        return "set-slot requires at least one field";
    while (i < argc) {
        const char *option = argv[i++];
        bool *present;
        uint8_t *value;
        unsigned max;
        if (strcmp(option, "--priority") == 0) {
            present = &request->update.has_priority;
            value = &request->update.priority;
            max = AVB_AB_MAX_PRIORITY;
        } else if (strcmp(option, "--tries") == 0) {
            present = &request->update.has_tries;
            value = &request->update.tries;
            max = AVB_AB_MAX_TRIES_REMAINING;
        } else if (strcmp(option, "--successful") == 0) {
            present = &request->update.has_successful;
            value = &request->update.successful;
            max = 1;
        } else {
            return "unknown set-slot option";
        }
        if (*present)
            return "duplicate set-slot option";
        if (i == argc || !parse_number(argv[i++], max, value))
            return "invalid field value: decimal priority=0..15, tries=0..7, successful=0..1";
        *present = true;
    }
    return NULL;
}

void cli_help(FILE *out)
{
    fputs("Usage: abctl [--misc PATH] COMMAND [ARGS]\n"
          "Default target: ./misc.img; metadata: offset 2048, length 32 bytes.\n"
          "Commands:\n"
          "  show\n"
          "  set-active SLOT\n"
          "  mark-successful SLOT\n"
          "  mark-unbootable SLOT\n"
          "  set-slot SLOT [--priority N] [--tries N] [--successful N]\n"
          "  init                  Reset metadata, including reserved bytes.\n"
          "  --help\n"
          "SLOT: a/b or 0/1. Fields: priority=0..15, tries=0..7, successful=0..1.\n"
          "set-slot accepts only disabled 0/0/0, trial 1..15/1..7/0, or\n"
          "successful 1..15/0/1 final states; unspecified fields are preserved.\n"
          "Examples:\n"
          "  abctl show\n"
          "  abctl --misc test.img set-active b\n"
          "  abctl set-slot a --priority 14 --tries 0 --successful 1\n"
          "Exit codes: 0=success, 1=operation/validation failure, 2=usage error.\n", out);
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
