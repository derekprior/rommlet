/*
 * Zip extraction module - Extract zip archives with progress reporting
 */

#include "zip.h"
#include "log.h"
#include <minizip/unzip.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define EXTRACT_CHUNK_SIZE (64 * 1024)

bool zip_is_zip_file(const char *filename) {
    if (!filename) return false;
    size_t len = strlen(filename);
    if (len < 4) return false;
    const char *ext = filename + len - 4;
    return (ext[0] == '.' && (ext[1] == 'z' || ext[1] == 'Z') && (ext[2] == 'i' || ext[2] == 'I') &&
            (ext[3] == 'p' || ext[3] == 'P'));
}

// Ensure all directories in a path exist
static bool ensure_parent_dirs(const char *filePath) {
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s", filePath);

    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    return true;
}

// Sum uncompressed sizes of all files in the archive
static uint32_t get_total_uncompressed_size(unzFile uf) {
    uint32_t total = 0;
    unz_file_info fileInfo;

    if (unzGoToFirstFile(uf) != UNZ_OK) return 0;

    do {
        if (unzGetCurrentFileInfo(uf, &fileInfo, NULL, 0, NULL, 0, NULL, 0) == UNZ_OK) {
            total += (uint32_t)fileInfo.uncompressed_size;
        }
    } while (unzGoToNextFile(uf) == UNZ_OK);

    return total;
}

bool zip_extract(const char *zipPath, const char *destDir, ExtractProgressCb progressCb) {
    unzFile uf = unzOpen(zipPath);
    if (!uf) {
        log_error("Failed to open zip: %s", zipPath);
        return false;
    }

    uint32_t totalSize = get_total_uncompressed_size(uf);
    uint32_t totalExtracted = 0;
    bool success = true;

    log_info("Extracting %s (%.1f MB uncompressed)", zipPath, totalSize / (1024.0f * 1024.0f));

    if (unzGoToFirstFile(uf) != UNZ_OK) {
        log_error("Failed to read first file in zip");
        unzClose(uf);
        return false;
    }

    unsigned char *buffer = malloc(EXTRACT_CHUNK_SIZE);
    if (!buffer) {
        log_error("Failed to allocate extraction buffer");
        unzClose(uf);
        return false;
    }

    do {
        char filename[256];
        unz_file_info fileInfo;

        if (unzGetCurrentFileInfo(uf, &fileInfo, filename, sizeof(filename), NULL, 0, NULL, 0) != UNZ_OK) {
            log_error("Failed to get file info from zip");
            success = false;
            break;
        }

        // Build destination path
        char destPath[512];
        snprintf(destPath, sizeof(destPath), "%s/%s", destDir, filename);

        // Skip directories (entries ending with /)
        size_t nameLen = strlen(filename);
        if (nameLen > 0 && filename[nameLen - 1] == '/') {
            mkdir(destPath, 0755);
            continue;
        }

        ensure_parent_dirs(destPath);

        if (unzOpenCurrentFile(uf) != UNZ_OK) {
            log_error("Failed to open file in zip: %s", filename);
            success = false;
            break;
        }

        FILE *outFile = fopen(destPath, "wb");
        if (!outFile) {
            log_error("Failed to create file: %s", destPath);
            unzCloseCurrentFile(uf);
            success = false;
            break;
        }

        int bytesRead;
        while ((bytesRead = unzReadCurrentFile(uf, buffer, EXTRACT_CHUNK_SIZE)) > 0) {
            if (fwrite(buffer, 1, bytesRead, outFile) != (size_t)bytesRead) {
                log_error("Failed to write extracted file: %s", destPath);
                success = false;
                break;
            }
            totalExtracted += bytesRead;

            if (progressCb) {
                if (!progressCb(totalExtracted, totalSize)) {
                    log_info("Extraction cancelled by user");
                    success = false;
                    break;
                }
            }
        }

        fclose(outFile);
        unzCloseCurrentFile(uf);

        if (bytesRead < 0) {
            log_error("Error reading from zip: %s", filename);
            success = false;
        }

        if (!success) break;

        log_debug("Extracted: %s", filename);
    } while (unzGoToNextFile(uf) == UNZ_OK);

    free(buffer);
    unzClose(uf);

    if (success) {
        remove(zipPath);
        log_info("Extraction complete, zip deleted");
    }

    return success;
}
