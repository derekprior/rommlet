/*
 * About screen - App info and sponsor link
 */

#ifndef ABOUT_H
#define ABOUT_H

#include <3ds.h>

typedef enum { ABOUT_NONE, ABOUT_BACK } AboutResult;

// Handle input, returns result
AboutResult about_update(u32 kDown);

// Draw the about screen
void about_draw(void);

#endif // ABOUT_H
