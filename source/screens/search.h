/*
 * Search screen - Search for ROMs across platforms
 */

#ifndef SEARCH_H
#define SEARCH_H

#include <3ds.h>
#include <stdbool.h>
#include "../api.h"

// Maximum platforms that can be filtered
#define SEARCH_MAX_PLATFORMS 128

typedef enum {
    SEARCH_FORM_NONE,
    SEARCH_FORM_BACK,
    SEARCH_FORM_EXECUTE
} SearchFormResult;

typedef enum {
    SEARCH_RESULTS_NONE,
    SEARCH_RESULTS_BACK,
    SEARCH_RESULTS_SELECTED,
    SEARCH_RESULTS_LOAD_MORE
} SearchResultsResult;

// Initialize search with available platforms
void search_init(Platform *platforms, int platformCount);

// Get the current search term
const char *search_get_term(void);

// Get selected platform IDs and count
const int *search_get_platform_ids(int *count);

// Set search result data (takes ownership of roms pointer)
void search_set_results(Rom *roms, int count, int total);

// Append more search results
void search_append_results(Rom *roms, int count);

// Get result count
int search_get_result_count(void);

// Get ROM at index from results
const Rom *search_get_result_at(int index);

// Get current selected index in results
int search_get_selected_index(void);

// Get ROM ID at index (returns -1 if invalid)
int search_get_result_id_at(int index);

// Update search form (bottom screen input)
SearchFormResult search_form_update(u32 kDown);

// Draw search form on bottom screen
void search_form_draw(void);

// Update search results (top screen navigation)
SearchResultsResult search_results_update(u32 kDown, int *outSelectedIndex);

// Draw search results on top screen
void search_results_draw(void);

// Open keyboard for search term (call on entry)
void search_open_keyboard(void);

// Look up platform display name by ID
const char *search_get_platform_name(int platformId);

// Look up platform slug by ID
const char *search_get_platform_slug(int platformId);

#endif // SEARCH_H
