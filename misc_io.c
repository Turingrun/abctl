#include "misc_io.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <stddef.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

int misc_io_open(const char *path, bool writable)
{
    /* Avoid blocking on an accidental FIFO path before checking its type. */
    int flags = (writable ? O_RDWR : O_RDONLY) | O_CLOEXEC | O_NONBLOCK;
    int fd;
    do {
        fd = open(path, flags);
    } while (fd < 0 && errno == EINTR);
    if (fd < 0)
        return -1;
    struct stat st;
    if (fstat(fd, &st) < 0)
        goto fail;
    if (!S_ISREG(st.st_mode) && !S_ISBLK(st.st_mode)) {
        errno = EINVAL;
        goto fail;
    }
    int rc;
    do {
        rc = flock(fd, writable ? LOCK_EX : LOCK_SH);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0)
        goto fail;
    uint64_t size;
    if (S_ISBLK(st.st_mode)) {
        do {
            rc = ioctl(fd, BLKGETSIZE64, &size);
        } while (rc < 0 && errno == EINTR);
        if (rc < 0)
            goto fail;
    } else {
        if (fstat(fd, &st) < 0)
            goto fail;
        size = (uint64_t)st.st_size;
    }
    if (size < MISC_METADATA_OFFSET + MISC_METADATA_SIZE) {
        errno = EINVAL;
        goto fail;
    }
    return fd;
fail:
    {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return -1;
    }
}

int misc_io_read(int fd, uint8_t raw[MISC_METADATA_SIZE])
{
    size_t done = 0;
    while (done < MISC_METADATA_SIZE) {
        ssize_t count = pread(fd, raw + done, MISC_METADATA_SIZE - done,
                              MISC_METADATA_OFFSET + (off_t)done);
        if (count < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (count == 0) {
            errno = EIO;
            return -1;
        }
        done += (size_t)count;
    }
    return 0;
}

int misc_io_write_verified(int fd, const uint8_t raw[MISC_METADATA_SIZE],
                           uint8_t readback[MISC_METADATA_SIZE])
{
    size_t done = 0;
    while (done < MISC_METADATA_SIZE) {
        ssize_t count = pwrite(fd, raw + done, MISC_METADATA_SIZE - done,
                               MISC_METADATA_OFFSET + (off_t)done);
        if (count < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (count == 0) {
            errno = EIO;
            return -1;
        }
        done += (size_t)count;
    }
    int rc;
    do {
        rc = fsync(fd);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0 || misc_io_read(fd, readback) < 0)
        return -1;
    if (memcmp(raw, readback, MISC_METADATA_SIZE) != 0) {
        errno = EIO;
        return -1;
    }
    return 0;
}

int misc_io_close(int fd)
{
    /* Linux may release fd even on EINTR; do not retry close. */
    return close(fd);
}
