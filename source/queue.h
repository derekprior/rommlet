/*
 * Download queue - In-memory queue for batch ROM downloads
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>

#define QUEUE_MAX_ENTRIES 64

typedef struct {
    int romId;
    int platformId;
    char name[256];
    char fsName[256];
    char platformSlug[64];
    char platformName[128];
    bool failed;
} QueueEntry;

// Initialize queue
void queue_init(void);

// Add a ROM to the queue. Returns true if added, false if full or duplicate.
bool queue_add(int romId, int platformId, const char *name, const char *fsName, const char *platformSlug,
               const char *platformName);

// Remove entry by romId. Returns true if found and removed.
bool queue_remove(int romId);

// Check if a ROM is already queued
bool queue_contains(int romId);

// Get queue count
int queue_count(void);

// Get entry at index (returns NULL if out of bounds)
QueueEntry *queue_get(int index);

// Mark entry as failed
void queue_set_failed(int index, bool failed);

// Clear all entries
void queue_clear(void);

// Clear failed flags (reset all to not-failed)
void queue_clear_failed(void);

#endif // QUEUE_H
