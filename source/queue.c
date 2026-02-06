/*
 * Download queue - In-memory queue for batch ROM downloads
 */

#include "queue.h"
#include <string.h>

static QueueEntry entries[QUEUE_MAX_ENTRIES];
static int entryCount = 0;

void queue_init(void) {
    entryCount = 0;
    memset(entries, 0, sizeof(entries));
}

bool queue_add(int romId, int platformId, const char *name, const char *fsName,
               const char *platformSlug, const char *platformName) {
    if (entryCount >= QUEUE_MAX_ENTRIES) return false;
    if (queue_contains(romId)) return false;

    QueueEntry *e = &entries[entryCount];
    e->romId = romId;
    e->platformId = platformId;
    strncpy(e->name, name, sizeof(e->name) - 1);
    strncpy(e->fsName, fsName, sizeof(e->fsName) - 1);
    strncpy(e->platformSlug, platformSlug, sizeof(e->platformSlug) - 1);
    strncpy(e->platformName, platformName, sizeof(e->platformName) - 1);
    e->failed = false;
    entryCount++;
    return true;
}

bool queue_remove(int romId) {
    for (int i = 0; i < entryCount; i++) {
        if (entries[i].romId == romId) {
            // Shift remaining entries down
            for (int j = i; j < entryCount - 1; j++) {
                entries[j] = entries[j + 1];
            }
            entryCount--;
            return true;
        }
    }
    return false;
}

bool queue_contains(int romId) {
    for (int i = 0; i < entryCount; i++) {
        if (entries[i].romId == romId) return true;
    }
    return false;
}

int queue_count(void) {
    return entryCount;
}

QueueEntry *queue_get(int index) {
    if (index < 0 || index >= entryCount) return NULL;
    return &entries[index];
}

void queue_set_failed(int index, bool failed) {
    if (index >= 0 && index < entryCount) {
        entries[index].failed = failed;
    }
}

void queue_clear(void) {
    entryCount = 0;
    memset(entries, 0, sizeof(entries));
}

void queue_clear_failed(void) {
    for (int i = 0; i < entryCount; i++) {
        entries[i].failed = false;
    }
}
