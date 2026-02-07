/*
 * Download queue - Persistent queue for batch ROM downloads
 */

#include "queue.h"
#include "config.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define QUEUE_PATH "sdmc:/3ds/rommlet/queue.txt"

static QueueEntry entries[QUEUE_MAX_ENTRIES];
static int entryCount = 0;

// Persist queue to SD card (tab-separated, one entry per line)
static void queue_save(void) {
    if (entryCount == 0) {
        remove(QUEUE_PATH);
        return;
    }
    FILE *f = fopen(QUEUE_PATH, "w");
    if (!f) {
        mkdir(CONFIG_DIR, 0755);
        f = fopen(QUEUE_PATH, "w");
    }
    if (!f) return;
    for (int i = 0; i < entryCount; i++) {
        fprintf(f, "%d\t%d\t%s\t%s\t%s\n", entries[i].romId, entries[i].platformId, entries[i].platformSlug,
                entries[i].fsName, entries[i].name);
    }
    if (ferror(f)) {
        log_error("Failed to write queue file");
    }
    fclose(f);
}

// Load queue from SD card
static void queue_load(void) {
    FILE *f = fopen(QUEUE_PATH, "r");
    if (!f) return;

    int skipped = 0;
    char line[1024];
    while (fgets(line, sizeof(line), f) && entryCount < QUEUE_MAX_ENTRIES) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        nl = strchr(line, '\r');
        if (nl) *nl = '\0';
        if (line[0] == '\0') continue;

        // Parse: romId \t platformId \t platformSlug \t fsName \t name
        char *fields[5];
        int fieldCount = 0;
        char *p = line;
        for (int i = 0; i < 4 && p; i++) {
            fields[fieldCount++] = p;
            p = strchr(p, '\t');
            if (p) *p++ = '\0';
        }
        if (p) fields[fieldCount++] = p;
        if (fieldCount < 5) {
            skipped++;
            continue;
        }

        QueueEntry *e = &entries[entryCount];
        e->romId = atoi(fields[0]);
        e->platformId = atoi(fields[1]);
        snprintf(e->platformSlug, sizeof(e->platformSlug), "%s", fields[2]);
        snprintf(e->fsName, sizeof(e->fsName), "%s", fields[3]);
        snprintf(e->name, sizeof(e->name), "%s", fields[4]);
        e->failed = false;
        entryCount++;
    }

    fclose(f);

    if (skipped > 0 && entryCount == 0) {
        log_error("Queue file corrupt, removing: %s", QUEUE_PATH);
        remove(QUEUE_PATH);
    } else if (entryCount > 0) {
        log_info("Loaded %d queued ROM(s) from disk", entryCount);
    }
}

void queue_init(void) {
    entryCount = 0;
    memset(entries, 0, sizeof(entries));
    queue_load();
}

bool queue_add(int romId, int platformId, const char *name, const char *fsName, const char *platformSlug) {
    if (entryCount >= QUEUE_MAX_ENTRIES) return false;
    if (queue_contains(romId)) return false;

    QueueEntry *e = &entries[entryCount];
    e->romId = romId;
    e->platformId = platformId;
    snprintf(e->name, sizeof(e->name), "%s", name);
    snprintf(e->fsName, sizeof(e->fsName), "%s", fsName);
    snprintf(e->platformSlug, sizeof(e->platformSlug), "%s", platformSlug);
    e->failed = false;
    entryCount++;
    queue_save();
    return true;
}

bool queue_remove(int romId) {
    for (int i = 0; i < entryCount; i++) {
        if (entries[i].romId == romId) {
            for (int j = i; j < entryCount - 1; j++) {
                entries[j] = entries[j + 1];
            }
            entryCount--;
            queue_save();
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
    queue_save();
}

void queue_clear_failed(void) {
    for (int i = 0; i < entryCount; i++) {
        entries[i].failed = false;
    }
}
