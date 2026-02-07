/*
 * Sound module - Synthesized UI sound effects via ndsp
 *
 * Generates click and pop PCM buffers at init time and plays them
 * on separate ndsp channels. Gracefully no-ops if the DSP firmware
 * (sdmc:/3ds/dspfirm.cdc) is not available.
 */

#include "sound.h"
#include <3ds.h>
#include <math.h>
#include <string.h>

#define SAMPLE_RATE 22050
#define CLICK_SAMPLES 1764 // ~80ms
#define POP_SAMPLES 1764   // ~80ms
#define AMPLITUDE 0.3f

#define CLICK_CHANNEL 0
#define POP_CHANNEL 1

static bool initialized = false;
static s16 *clickBuf = NULL;
static s16 *popBuf = NULL;
static ndspWaveBuf clickWaveBuf;
static ndspWaveBuf popWaveBuf;

static void generate_click(void) {
    float phase = 0.0f;
    for (int i = 0; i < CLICK_SAMPLES; i++) {
        float progress = (float)i / CLICK_SAMPLES;
        float env = 1.0f - progress;
        env *= env;
        float freq = 300.0f + 500.0f * progress;
        phase += freq / SAMPLE_RATE;
        float wave = sinf(2.0f * M_PI * phase);
        clickBuf[i] = (s16)(wave * env * AMPLITUDE * 32767);
    }
}

static void generate_pop(void) {
    float phase = 0.0f;
    for (int i = 0; i < POP_SAMPLES; i++) {
        float progress = (float)i / POP_SAMPLES;
        float env = 1.0f - progress;
        env *= env;
        float freq = 800.0f - 500.0f * progress;
        phase += freq / SAMPLE_RATE;
        float wave = sinf(2.0f * M_PI * phase);
        popBuf[i] = (s16)(wave * env * AMPLITUDE * 32767);
    }
}

void sound_init(void) {
    if (ndspInit() != 0) return;

    clickBuf = (s16 *)linearAlloc(CLICK_SAMPLES * sizeof(s16));
    popBuf = (s16 *)linearAlloc(POP_SAMPLES * sizeof(s16));
    if (!clickBuf || !popBuf) {
        if (clickBuf) linearFree(clickBuf);
        if (popBuf) linearFree(popBuf);
        ndspExit();
        return;
    }

    generate_click();
    generate_pop();
    DSP_FlushDataCache(clickBuf, CLICK_SAMPLES * sizeof(s16));
    DSP_FlushDataCache(popBuf, POP_SAMPLES * sizeof(s16));

    ndspChnSetInterp(CLICK_CHANNEL, NDSP_INTERP_LINEAR);
    ndspChnSetRate(CLICK_CHANNEL, SAMPLE_RATE);
    ndspChnSetFormat(CLICK_CHANNEL, NDSP_FORMAT_MONO_PCM16);

    ndspChnSetInterp(POP_CHANNEL, NDSP_INTERP_LINEAR);
    ndspChnSetRate(POP_CHANNEL, SAMPLE_RATE);
    ndspChnSetFormat(POP_CHANNEL, NDSP_FORMAT_MONO_PCM16);

    memset(&clickWaveBuf, 0, sizeof(clickWaveBuf));
    clickWaveBuf.data_vaddr = clickBuf;
    clickWaveBuf.nsamples = CLICK_SAMPLES;

    memset(&popWaveBuf, 0, sizeof(popWaveBuf));
    popWaveBuf.data_vaddr = popBuf;
    popWaveBuf.nsamples = POP_SAMPLES;

    initialized = true;
}

void sound_exit(void) {
    if (!initialized) return;
    ndspChnWaveBufClear(CLICK_CHANNEL);
    ndspChnWaveBufClear(POP_CHANNEL);
    linearFree(clickBuf);
    linearFree(popBuf);
    ndspExit();
    initialized = false;
}

void sound_play_click(void) {
    if (!initialized) return;
    ndspChnWaveBufClear(CLICK_CHANNEL);
    clickWaveBuf.status = NDSP_WBUF_FREE;
    ndspChnWaveBufAdd(CLICK_CHANNEL, &clickWaveBuf);
}

void sound_play_pop(void) {
    if (!initialized) return;
    ndspChnWaveBufClear(POP_CHANNEL);
    popWaveBuf.status = NDSP_WBUF_FREE;
    ndspChnWaveBufAdd(POP_CHANNEL, &popWaveBuf);
}
