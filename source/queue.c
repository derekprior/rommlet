/*
 * Download queue - In-memory queue for batch ROM downloads
 */

#include "queue.h"
#include <stdio.h>
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
    snprintf(e->name, sizeof(e->name), "%s", name);
    snprintf(e->fsName, sizeof(e->fsName), "%s", fsName);
    snprintf(e->platformSlug, sizeof(e->platformSlug), "%s", platformSlug);
    snprintf(e->platformName, sizeof(e->platformName), "%s", platformName);
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
