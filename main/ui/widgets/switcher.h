#pragma once

// The bottom "<  Name  >" pill with a dot per item, used to switch between Home's shelves and the
// Library's views. Tap an arrow or drag at least 20 px left/right on the pill. A small progress cue
// appears while dragging, and the swipe stays local to the pill.

#include <stdbool.h>
#include "lvgl.h"

#define SWITCHER_MAX 6
// The pill's size, for pages that show a plain title in the same place.
#define SWITCHER_W 250
#define SWITCHER_H 44

typedef struct switcher switcher_t;

// Called when the user moves to another item; `dir` is -1 or +1. The owner shows the item and
// then calls switcher_select().
typedef void (*switcher_cb_t)(int index, int dir);

switcher_t *switcher_create(lv_obj_t *parent, int y, const char *const *names, int count, switcher_cb_t cb);
// Disabled items are skipped by the arrows and have no dot (e.g. an empty shelf).
void switcher_set_enabled(switcher_t *sw, int index, bool enabled);
bool switcher_is_enabled(const switcher_t *sw, int index);
// Next enabled item in `dir` from `from`, or -1.
int switcher_next(const switcher_t *sw, int from, int dir);
void switcher_select(switcher_t *sw, int index);
