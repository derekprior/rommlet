/*
 * Background loader module - Async ROM fetching
 */

#include "loader.h"
#include "api.h"
#include <3ds.h>
#include <stdlib.h>
#include <string.h>

#define LOADER_STACK_SIZE (16 * 1024)

// Loader state
typedef enum {
    LOADER_IDLE,
    LOADER_WORKING,
    LOADER_COMPLETE,
    LOADER_FAILED
} LoaderState;

// Request parameters
typedef struct {
    int platformId;
    int offset;
    int limit;
} LoaderRequest;

// Loader context
static Thread loaderThread = NULL;
static LightLock stateLock;
static volatile LoaderState state = LOADER_IDLE;
static LoaderRequest request;
static Rom *resultRoms = NULL;
static int resultCount = 0;
static int resultTotal = 0;

static void loader_thread_func(void *arg) {
    (void)arg;
    
    // Fetch ROMs
    int count = 0;
    int total = 0;
    Rom *roms = api_get_roms(request.platformId, request.offset, request.limit, &count, &total);
    
    // Store results
    LightLock_Lock(&stateLock);
    resultRoms = roms;
    resultCount = count;
    resultTotal = total;
    state = roms ? LOADER_COMPLETE : LOADER_FAILED;
    LightLock_Unlock(&stateLock);
}

void loader_init(void) {
    LightLock_Init(&stateLock);
    state = LOADER_IDLE;
    loaderThread = NULL;
    resultRoms = NULL;
    resultCount = 0;
    resultTotal = 0;
}

void loader_exit(void) {
    // Wait for any pending thread
    if (loaderThread) {
        threadJoin(loaderThread, U64_MAX);
        threadFree(loaderThread);
        loaderThread = NULL;
    }
    
    // Free any unclaimed results
    if (resultRoms) {
        free(resultRoms);
        resultRoms = NULL;
    }
}

bool loader_start_roms(int platformId, int offset, int limit) {
    LightLock_Lock(&stateLock);
    
    // Can't start if already working
    if (state == LOADER_WORKING) {
        LightLock_Unlock(&stateLock);
        return false;
    }
    
    // Clean up previous thread if any
    if (loaderThread) {
        threadJoin(loaderThread, U64_MAX);
        threadFree(loaderThread);
        loaderThread = NULL;
    }
    
    // Free any unclaimed previous results
    if (resultRoms) {
        free(resultRoms);
        resultRoms = NULL;
    }
    
    // Set up request
    request.platformId = platformId;
    request.offset = offset;
    request.limit = limit;
    resultCount = 0;
    resultTotal = 0;
    state = LOADER_WORKING;
    
    LightLock_Unlock(&stateLock);
    
    // Create and start thread
    s32 prio = 0;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    loaderThread = threadCreate(loader_thread_func, NULL, LOADER_STACK_SIZE, prio + 1, -2, true);
    
    if (!loaderThread) {
        LightLock_Lock(&stateLock);
        state = LOADER_FAILED;
        LightLock_Unlock(&stateLock);
        return false;
    }
    
    return true;
}

bool loader_is_complete(void) {
    LightLock_Lock(&stateLock);
    bool complete = (state == LOADER_COMPLETE);
    LightLock_Unlock(&stateLock);
    return complete;
}

bool loader_is_busy(void) {
    LightLock_Lock(&stateLock);
    bool busy = (state == LOADER_WORKING);
    LightLock_Unlock(&stateLock);
    return busy;
}

Rom *loader_get_roms(int *count, int *total) {
    LightLock_Lock(&stateLock);
    
    if (state != LOADER_COMPLETE) {
        LightLock_Unlock(&stateLock);
        *count = 0;
        *total = 0;
        return NULL;
    }
    
    // Transfer ownership
    Rom *roms = resultRoms;
    *count = resultCount;
    *total = resultTotal;
    
    resultRoms = NULL;
    resultCount = 0;
    resultTotal = 0;
    state = LOADER_IDLE;
    
    LightLock_Unlock(&stateLock);
    return roms;
}
