#include "ab_metadata.h"
#include "ab_slot.h"
#include "cli.h"
#include "misc_io.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

_Static_assert(MISC_METADATA_SIZE == AVB_AB_DATA_SIZE, "I/O size mismatch");

static AbSlotResult apply_request(const CliRequest *request, AvbABData *data)
{
    switch (request->command) {
    case CLI_INIT:
        ab_metadata_init(data);
        return AB_SLOT_OK;
    case CLI_ACTIVE:
        return ab_slot_apply(data, request->slot, AB_SLOT_ACTIVE);
    case CLI_SUCCESSFUL:
        return ab_slot_apply(data, request->slot, AB_SLOT_SUCCESSFUL);
    case CLI_UNBOOTABLE:
        return ab_slot_apply(data, request->slot, AB_SLOT_UNBOOTABLE);
    case CLI_SET_SLOT:
        return ab_slot_update(data, request->slot, &request->update);
    default:
        return AB_SLOT_INVALID_ARGUMENT;
    }
}

int main(int argc, char **argv)
{
    CliRequest request;
    const char *error = cli_parse(argc, argv, &request);
    if (error != NULL) {
        fprintf(stderr, "abctl: %s\n", error);
        cli_help(stderr);
        return 2;
    }
    if (request.command == CLI_HELP) {
        cli_help(stdout);
        return 0;
    }
    bool writable = request.command != CLI_SHOW;
    int fd = misc_io_open(request.path, writable);
    if (fd < 0) {
        fprintf(stderr, "abctl: open/lock/size check '%s': %s "
                "(expected a regular file or block device of at least 2080 bytes)\n",
                request.path, strerror(errno));
        return 1;
    }
    int result = 1;
    bool write_attempted = false;
    uint8_t original[AVB_AB_DATA_SIZE], updated[AVB_AB_DATA_SIZE], readback[AVB_AB_DATA_SIZE];
    AvbABData data;
    AbValidation validation;
    if (misc_io_read(fd, original) < 0) {
        fprintf(stderr, "abctl: read metadata: %s\n", strerror(errno));
        goto out;
    }
    bool valid = ab_metadata_decode(original, &data, &validation);
    if (!writable) {
        cli_show(request.path, &data, &validation);
        if (!valid)
            cli_validation_errors(&validation);
        result = valid ? 0 : 1;
        goto out;
    }
    if (!valid && request.command != CLI_INIT) {
        cli_validation_errors(&validation);
        goto out;
    }
    AbSlotResult slot_result = apply_request(&request, &data);
    if (slot_result != AB_SLOT_OK) {
        const char *reason = slot_result == AB_SLOT_NOT_BOOTABLE ?
            "cannot mark an unbootable slot successful" :
            slot_result == AB_SLOT_INVALID_COMBINATION ?
            "invalid final slot combination; supply matching fields in one set-slot command" :
            "invalid slot operation";
        fprintf(stderr, "abctl: %s\n", reason);
        goto out;
    }
    ab_metadata_encode(&data, updated);
    if (memcmp(original, updated, sizeof(updated)) != 0) {
        write_attempted = true;
        if (misc_io_write_verified(fd, updated, readback) < 0) {
            fprintf(stderr, "abctl: write/sync/readback failed: %s; "
                    "target may have been modified, re-read before retrying\n", strerror(errno));
            goto out;
        }
        if (!ab_metadata_decode(readback, &data, &validation)) {
            cli_validation_errors(&validation);
            fputs("abctl: readback invalid; target may have been modified\n", stderr);
            goto out;
        }
    } else {
        (void)ab_metadata_decode(updated, &data, &validation);
    }
    result = 0;
out:
    if (misc_io_close(fd) < 0) {
        fprintf(stderr, "abctl: close failed: %s%s\n", strerror(errno),
                write_attempted ? "; target may have been modified" : "");
        result = 1;
    }
    if (result == 0 && writable) {
        puts(write_attempted ? "metadata updated and verified" : "metadata unchanged");
        cli_show(request.path, &data, &validation);
    }
    if (fflush(stdout) == EOF || ferror(stdout)) {
        fputs("abctl: output failed\n", stderr);
        result = 1;
    }
    return result;
}
