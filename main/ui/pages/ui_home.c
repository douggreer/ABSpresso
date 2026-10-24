// Home: shelves of books, one at a time. Drag left/right on a cover to browse; change shelf with
// the switcher pill at the bottom. Search-by-title and search-by-author occupy their own shelves.

#include "carousel.h"
#include "switcher.h"
#include "ui_priv.h"

typedef struct {
    const book_list_t *books;
    int pos;  // remembered position within the shelf
} row_t;

static const char *const s_names[] = {
    "Continue Listening", "Search by Title", "Search by Author", "Recently Added", "Downloaded",
};
static row_t s_rows[] = {{&g_continue, 0}, {NULL, 0}, {NULL, 0}, {&g_recent, 0}, {&g_downloaded, 0}};
#define ROW_COUNT (int)(sizeof(s_rows) / sizeof(s_rows[0]))

static lv_obj_t *s_group, *s_empty, *s_search_prompt, *s_search_title, *s_search_hint;
static lv_obj_t *s_search_button, *s_search_button_text;
static carousel_t *s_carousel;
static switcher_t *s_switcher;
static int s_row = -1;

static bool row_has_books(int r)
{
    if (r < 0 || r >= ROW_COUNT) return false;
    if (r == 1 || r == 2) return true;
    return s_rows[r].books && s_rows[r].books->count > 0;
}

static bool row_has_carousel(int r)
{
    return r >= 0 && r < ROW_COUNT && s_rows[r].books != NULL;
}

static int first_row(void)
{
    if (row_has_books(0)) return 0;
    if (row_has_books(3)) return 3;
    if (row_has_books(4)) return 4;
    return 1;
}

static void group_set_x(void *obj, int32_t x)
{
    lv_obj_set_x(obj, x);
}

static void show_row(int r, int dir)
{
    if (row_has_carousel(s_row)) {
        s_rows[s_row].pos = carousel_pos(s_carousel);
    }
    s_row = r;
    if (r < 0 || r >= ROW_COUNT) {
        lv_obj_add_flag(s_group, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_search_prompt, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_empty, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_add_flag(s_empty, LV_OBJ_FLAG_HIDDEN);
    for (int i = 0; i < ROW_COUNT; i++) switcher_set_enabled(s_switcher, i, row_has_books(i));
    switcher_select(s_switcher, r);

    if (!row_has_carousel(r)) {
        lv_obj_add_flag(s_group, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_search_prompt, LV_OBJ_FLAG_HIDDEN);
        bool by_author = r == 2;
        lv_label_set_text(s_search_title, by_author ? "Search by author" : "Search by title");
        lv_label_set_text(s_search_hint, by_author ? "Find books by author" : "Find books by title");
        lv_label_set_text(s_search_button_text, "Tap to search");
        return;
    }

    lv_obj_add_flag(s_search_prompt, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_group, LV_OBJ_FLAG_HIDDEN);
    carousel_set_items(s_carousel, s_rows[r].books->idx, s_rows[r].books->count, s_rows[r].pos);

    if (dir) {
        // Slide the new shelf in from the side it came from.
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, s_group);
        lv_anim_set_exec_cb(&a, group_set_x);
        lv_anim_set_values(&a, dir * 40, 0);
        lv_anim_set_duration(&a, 200);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_start(&a);
    }
}

static void on_switch(int index, int dir)
{
    show_row(index, dir);
}

static void on_search(lv_event_t *e)
{
    (void)e;
    ui_search_open(s_row == 2);
}

void home_build(lv_obj_t *page)
{
    s_group = lv_obj_create(page);
    lv_obj_remove_style_all(s_group);
    lv_obj_set_size(s_group, 360, 360);
    lv_obj_remove_flag(s_group, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(s_group, LV_OBJ_FLAG_CLICKABLE);

    // No ring on Home, so the carousel can use the full width.
    const carousel_cfg_t cfg = {.y = PAGE_CAROUSEL_Y, .x = 0, .width = 360, .title_w = 240, .sub_w = 230};
    s_carousel = carousel_create(s_group, &cfg);
    s_switcher = switcher_create(page, PAGE_SWITCHER_Y, s_names, ROW_COUNT, on_switch);

    s_search_prompt = lv_obj_create(page);
    lv_obj_remove_style_all(s_search_prompt);
    lv_obj_set_size(s_search_prompt, 270, 148);
    lv_obj_align(s_search_prompt, LV_ALIGN_TOP_MID, 0, PAGE_CAROUSEL_Y + 16);
    lv_obj_set_style_bg_color(s_search_prompt, COLOR_CARD, 0);
    lv_obj_set_style_bg_opa(s_search_prompt, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_search_prompt, 24, 0);
    lv_obj_remove_flag(s_search_prompt, LV_OBJ_FLAG_SCROLLABLE);

    s_search_title = ui_label(s_search_prompt, &ui_font_20, COLOR_TEXT, 248);
    lv_obj_align(s_search_title, LV_ALIGN_TOP_MID, 0, 20);
    s_search_hint = ui_label(s_search_prompt, &ui_font_14, COLOR_MUTED, 248);
    lv_obj_align(s_search_hint, LV_ALIGN_TOP_MID, 0, 58);
    s_search_button = lv_button_create(s_search_prompt);
    lv_obj_set_size(s_search_button, 210, 44);
    lv_obj_align(s_search_button, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_obj_set_style_radius(s_search_button, 18, 0);
    lv_obj_set_style_bg_color(s_search_button, COLOR_ACCENT, 0);
    lv_obj_set_style_bg_color(s_search_button, COLOR_TEXT, LV_STATE_PRESSED);
    s_search_button_text = ui_label(s_search_button, &ui_font_16, lv_color_black(), 190);
    lv_label_set_text(s_search_button_text, "Tap to search");
    lv_obj_center(s_search_button_text);
    lv_obj_add_event_cb(s_search_button, on_search, LV_EVENT_CLICKED, NULL);

    s_empty = ui_label(page, &ui_font_16, COLOR_MUTED, 240);
    lv_label_set_long_mode(s_empty, LV_LABEL_LONG_WRAP);
    lv_label_set_text(s_empty, "Nothing here yet.\nBrowse the " LV_SYMBOL_LIST " library to start a book.");
    lv_obj_center(s_empty);
    lv_obj_add_flag(s_empty, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_group, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_search_prompt, LV_OBJ_FLAG_HIDDEN);
}

void home_set_books(void)
{
    // New data: start each shelf from its first book, on the first non-empty shelf.
    for (int i = 0; i < ROW_COUNT; i++) s_rows[i].pos = 0;
    s_row = -1;
    show_row(first_row(), 0);
}

void home_lists_changed(void)
{
    // Stay on the current shelf if it still has books (keeping the position); else find another.
    int r = row_has_books(s_row) ? s_row : first_row();
    if (row_has_carousel(s_row)) s_rows[s_row].pos = carousel_pos(s_carousel);
    s_row = -1;
    show_row(r, 0);
}

void home_refresh(void)
{
    if (row_has_carousel(s_row)) carousel_refresh(s_carousel);
}

#ifdef UI_CAPTURE
void home_debug_next_row(void)
{
    int r = switcher_next(s_switcher, s_row, 1);
    if (r >= 0) show_row(r, 1);
}
#endif
