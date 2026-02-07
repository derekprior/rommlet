/*
 * Sound module - Synthesized UI sound effects via ndsp
 */

#ifndef SOUND_H
#define SOUND_H

// Initialize audio (silently no-ops if DSP firmware unavailable)
void sound_init(void);

// Cleanup audio resources
void sound_exit(void);

// Play a short click for forward/confirm actions
void sound_play_click(void);

// Play a short pop for back/cancel actions
void sound_play_pop(void);

#endif // SOUND_H
