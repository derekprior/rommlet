/*
 * Queue screen - Display queued ROMs for batch download
 */

#ifndef QUEUE_SCREEN_H
#define QUEUE_SCREEN_H

#include <3ds.h>
#include "../queue.h"

typedef enum { QUEUE_NONE, QUEUE_BACK, QUEUE_SELECTED } QueueResult;

// Initialize queue screen
void queue_screen_init(void);

// Update queue screen, returns result
QueueResult queue_screen_update(u32 kDown);

// Get current selected index
int queue_screen_get_selected_index(void);

// Draw queue screen on top screen
void queue_screen_draw(void);

#endif // QUEUE_SCREEN_H
