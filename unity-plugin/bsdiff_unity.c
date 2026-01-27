/*
 * bsdiff/bspatch Unity wrapper
 * File-based API for Unity P/Invoke
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "../bsdiff.h"
#include "../bspatch.h"

#if defined(_WIN32)
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT __attribute__((visibility("default")))
#endif

/* ========== bspatch (patch application) ========== */

static int file_read(const struct bspatch_stream* stream, void* buffer, int length) {
    FILE* f = (FILE*)stream->opaque;
    if (fread(buffer, 1, length, f) != (size_t)length)
        return -1;
    return 0;
}

static int64_t offtin(const uint8_t *buf) {
    int64_t y;
    y = buf[7] & 0x7F;
    y = y * 256 + buf[6];
    y = y * 256 + buf[5];
    y = y * 256 + buf[4];
    y = y * 256 + buf[3];
    y = y * 256 + buf[2];
    y = y * 256 + buf[1];
    y = y * 256 + buf[0];
    if (buf[7] & 0x80) y = -y;
    return y;
}

/*
 * Apply a patch file to create a new file.
 * Returns: 0 on success, -1 on error
 */
EXPORT int bspatch_file(const char* old_path, const char* new_path, const char* patch_path) {
    FILE *pf = NULL;
    int fd = -1;
    uint8_t header[16];
    uint8_t *old_data = NULL, *new_data = NULL;
    int64_t old_size, new_size;
    struct stat sb;
    int result = -1;

    /* Open patch file */
    pf = fopen(patch_path, "rb");
    if (!pf) goto cleanup;

    /* Read header */
    if (fread(header, 1, 16, pf) != 16) goto cleanup;
    if (memcmp(header, "BSDIFFRA", 8) != 0) goto cleanup;

    new_size = offtin(header + 8);
    if (new_size < 0) goto cleanup;

    /* Read old file */
    fd = open(old_path, O_RDONLY);
    if (fd < 0) goto cleanup;
    if (fstat(fd, &sb) < 0) goto cleanup;
    old_size = sb.st_size;

    old_data = malloc(old_size);
    if (!old_data) goto cleanup;
    if (read(fd, old_data, old_size) != old_size) goto cleanup;
    close(fd);
    fd = -1;

    /* Allocate new buffer */
    new_data = malloc(new_size);
    if (!new_data) goto cleanup;

    /* Apply patch */
    struct bspatch_stream stream;
    stream.opaque = pf;
    stream.read = file_read;
    if (bspatch(old_data, old_size, new_data, new_size, &stream) != 0) goto cleanup;

    /* Write new file */
    fd = open(new_path, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fd < 0) goto cleanup;
    if (write(fd, new_data, new_size) != new_size) goto cleanup;

    result = 0;

cleanup:
    if (fd >= 0) close(fd);
    if (pf) fclose(pf);
    free(old_data);
    free(new_data);
    return result;
}

/* ========== bsdiff (diff generation) ========== */

static void* stream_malloc(size_t size) { return malloc(size); }
static void stream_free(void* ptr) { free(ptr); }

static int file_write(struct bsdiff_stream* stream, const void* buffer, int size) {
    FILE* f = (FILE*)stream->opaque;
    if (fwrite(buffer, 1, size, f) != (size_t)size)
        return -1;
    return 0;
}

static void offout(int64_t x, uint8_t* buf) {
    int64_t y = (x < 0) ? -x : x;
    buf[0] = y % 256; y /= 256;
    buf[1] = y % 256; y /= 256;
    buf[2] = y % 256; y /= 256;
    buf[3] = y % 256; y /= 256;
    buf[4] = y % 256; y /= 256;
    buf[5] = y % 256; y /= 256;
    buf[6] = y % 256; y /= 256;
    buf[7] = y % 256;
    if (x < 0) buf[7] |= 0x80;
}

/*
 * Create a patch file from old and new files.
 * Returns: 0 on success, -1 on error
 */
EXPORT int bsdiff_file(const char* old_path, const char* new_path, const char* patch_path) {
    int fd = -1;
    FILE *pf = NULL;
    uint8_t *old_data = NULL, *new_data = NULL;
    int64_t old_size, new_size;
    struct stat sb;
    uint8_t header[16];
    int result = -1;

    /* Read old file */
    fd = open(old_path, O_RDONLY);
    if (fd < 0) goto cleanup;
    if (fstat(fd, &sb) < 0) goto cleanup;
    old_size = sb.st_size;
    old_data = malloc(old_size);
    if (!old_data) goto cleanup;
    if (read(fd, old_data, old_size) != old_size) goto cleanup;
    close(fd);

    /* Read new file */
    fd = open(new_path, O_RDONLY);
    if (fd < 0) goto cleanup;
    if (fstat(fd, &sb) < 0) goto cleanup;
    new_size = sb.st_size;
    new_data = malloc(new_size);
    if (!new_data) goto cleanup;
    if (read(fd, new_data, new_size) != new_size) goto cleanup;
    close(fd);
    fd = -1;

    /* Open patch file */
    pf = fopen(patch_path, "wb");
    if (!pf) goto cleanup;

    /* Write header */
    memcpy(header, "BSDIFFRA", 8);
    offout(new_size, header + 8);
    if (fwrite(header, 1, 16, pf) != 16) goto cleanup;

    /* Generate patch */
    struct bsdiff_stream stream;
    stream.opaque = pf;
    stream.malloc = stream_malloc;
    stream.free = stream_free;
    stream.write = file_write;
    if (bsdiff(old_data, old_size, new_data, new_size, &stream) != 0) goto cleanup;

    result = 0;

cleanup:
    if (fd >= 0) close(fd);
    if (pf) fclose(pf);
    free(old_data);
    free(new_data);
    return result;
}