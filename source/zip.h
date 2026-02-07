/*
 * Zip extraction module - Extract zip archives with progress reporting
 */

#ifndef ZIP_H
#define ZIP_H

#include <stdbool.h>
#include <stdint.h>

// Progress callback for extraction. Returns true to continue, false to cancel.
// extracted: bytes extracted so far, total: total uncompressed size
typedef bool (*ExtractProgressCb)(uint32_t extracted, uint32_t total);

// Extract all files from a zip archive into destDir.
// Deletes the zip file on success.
// Returns true on success, false on failure or cancellation.
bool zip_extract(const char *zipPath, const char *destDir, ExtractProgressCb progressCb);

// Check if a filename has a .zip extension (case-insensitive)
bool zip_is_zip_file(const char *filename);

#endif // ZIP_H
