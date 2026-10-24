// Full-screen local title/author search for the round 240x240 display.

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "ui_priv.h"

#define TITLE_RESULT_LIMIT 12

static lv_obj_t *s_overlay, *s_back, *s_query, *s_author_title, *s_results, *s_keyboard;
static bool s_visible, s_by_author, s_author_books_view;

static bool contains_ci(const char *text, const char *query)
{
    size_t n = strlen(query);
    if (!n) return true;
    for (const char *p = text; p && *p; p++) {
        if (strncasecmp(p, query, n) == 0) return true;
    }
    return false;
}

static int selected_author_index(void)
{
    const char *selected_name = lv_label_get_text(s_author_title);
    if (!selected_name || !selected_name[0]) return -1;
    for (int i = 0; i < g_author_count; i++) {
        if (strcasecmp(g_authors[i].name, selected_name) == 0) return i;
    }
    return -1;
}

static void add_empty_message(const char *text)
{
    lv_obj_t *label = lv_label_create(s_results);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &ui_font_14, 0);
    lv_obj_set_style_text_color(label, COLOR_MUTED, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    ui_obj_set_width_raw(label, 190);
    lv_obj_center(label);
}

static void on_book_result(lv_event_t *e)
{
    int index = (int)(intptr_t)lv_event_get_user_data(e);
    ui_search_close();
    ui_open_book(index);
}

static void add_book_result(int index)
{
    const abs_book_t *book = &g_books[index];
    lv_obj_t *row = lv_button_create(s_results);
    ui_obj_set_size_raw(row, 200, 30);
    lv_obj_set_style_bg_color(row, COLOR_CARD, 0);
    lv_obj_set_style_bg_color(row, COLOR_ACCENT, LV_STATE_PRESSED);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_radius(row, 8, 0);
    lv_obj_set_style_pad_all(row, 2, 0);

    lv_obj_t *label = lv_label_create(row);
    // Keep search results to one line so long titles are truncated from the end instead of
    // wrapping and hiding their beginning at the round edge of the display.
    lv_label_set_text(label, book->title);
    ui_one_line(label, &ui_font_14);
    lv_obj_set_style_text_color(label, COLOR_TEXT, 0);
    ui_obj_set_width_raw(label, 190);
    lv_obj_center(label);
    lv_obj_add_event_cb(row, on_book_result, LV_EVENT_CLICKED, (void *)(intptr_t)index);
}

static void on_author_result(lv_event_t *e)
{
    int index = (int)(intptr_t)lv_event_get_user_data(e);
    if (index < 0 || index >= g_author_count) return;

    s_author_books_view = true;
    lv_label_set_text(s_author_title, g_authors[index].name);
    lv_obj_add_flag(s_query, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_author_title, LV_OBJ_FLAG_HIDDEN);
    ui_obj_set_pos_raw(s_results, 16, 62);
    ui_obj_set_size_raw(s_results, 208, 172);
    search_refresh();
}

static void add_author_result(int index)
{
    const author_t *author = &g_authors[index];
    lv_obj_t *row = lv_button_create(s_results);
    ui_obj_set_size_raw(row, 200, 30);
    lv_obj_set_style_bg_color(row, COLOR_CARD, 0);
    lv_obj_set_style_bg_color(row, COLOR_ACCENT, LV_STATE_PRESSED);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_radius(row, 8, 0);
    lv_obj_set_style_pad_all(row, 2, 0);

    lv_obj_t *name = lv_label_create(row);
    lv_label_set_text(name, author->name);
    lv_obj_set_style_text_font(name, &ui_font_14, 0);
    lv_obj_set_style_text_color(name, COLOR_TEXT, 0);
    lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
    ui_obj_set_width_raw(name, 156);
    lv_obj_align(name, LV_ALIGN_LEFT_MID, 8, 0);

    char count[16];
    snprintf(count, sizeof(count), "%d", author->books.count);
    lv_obj_t *book_count = lv_label_create(row);
    lv_label_set_text(book_count, count);
    lv_obj_set_style_text_font(book_count, &ui_font_14, 0);
    lv_obj_set_style_text_color(book_count, COLOR_MUTED, 0);
    lv_obj_align(book_count, LV_ALIGN_RIGHT_MID, -8, 0);

    lv_obj_add_event_cb(row, on_author_result, LV_EVENT_CLICKED, (void *)(intptr_t)index);
}

static void refresh_results(void)
{
    if (!s_results) return;
    lv_obj_clean(s_results);

    if (s_author_books_view) {
        int author = selected_author_index();
        if (author < 0) {
            s_author_books_view = false;
            lv_label_set_text(s_author_title, "");
            lv_obj_add_flag(s_author_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(s_query, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(s_keyboard, LV_OBJ_FLAG_HIDDEN);
            ui_obj_set_pos_raw(s_results, 20, 66);
            ui_obj_set_size_raw(s_results, 200, 62);
            add_empty_message("Author list updated. Search again.");
            return;
        }

        const book_list_t *books = &g_authors[author].books;
        for (int i = 0; i < books->count; i++) add_book_result(books->idx[i]);
        if (!books->count) add_empty_message("No books for this author");
        lv_obj_scroll_to_y(s_results, 0, LV_ANIM_OFF);
        return;
    }

    const char *query = lv_textarea_get_text(s_query);
    if (!query || strlen(query) < 2) {
        add_empty_message("Type 2 or more letters");
        return;
    }

    if (s_by_author) {
        int matches = 0;
        for (int i = 0; i < g_author_count; i++) {
            if (!contains_ci(g_authors[i].name, query)) continue;
            add_author_result(i);
            matches++;
        }
        if (!matches) add_empty_message("No matching authors");
    } else {
        int matches = 0;
        for (int i = 0; i < g_book_count; i++) {
            if (!contains_ci(g_books[i].title, query)) continue;
            if (matches < TITLE_RESULT_LIMIT) add_book_result(i);
            matches++;
        }
        if (!matches) add_empty_message("No matching books");
    }
    lv_obj_scroll_to_y(s_results, 0, LV_ANIM_OFF);
}

static void on_query_changed(lv_event_t *e)
{
    (void)e;
    refresh_results();
}

static void on_back(lv_event_t *e)
{
    (void)e;
    if (!s_author_books_view) {
        ui_search_close();
        return;
    }

    s_author_books_view = false;
    lv_label_set_text(s_author_title, "");
    lv_obj_add_flag(s_author_title, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_query, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_keyboard, LV_OBJ_FLAG_HIDDEN);
    ui_obj_set_pos_raw(s_results, 20, 66);
    ui_obj_set_size_raw(s_results, 200, 62);
    refresh_results();
}

void search_build(lv_obj_t *scr)
{
    s_overlay = ui_page_container(scr);
    lv_obj_set_style_bg_color(s_overlay, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_COVER, 0);
    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_CLICKABLE);

    s_back = lv_button_create(s_overlay);
    ui_obj_set_pos_raw(s_back, 100, 4);
    ui_obj_set_size_raw(s_back, 40, 32);
    lv_obj_set_style_bg_color(s_back, COLOR_CARD, 0);
    lv_obj_set_style_bg_color(s_back, COLOR_ACCENT, LV_STATE_PRESSED);
    lv_obj_set_style_border_width(s_back, 0, 0);
    lv_obj_set_style_radius(s_back, LV_RADIUS_CIRCLE, 0);
    lv_obj_t *back_label = lv_label_create(s_back);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(back_label, COLOR_TEXT, 0);
    lv_obj_center(back_label);
    lv_obj_add_event_cb(s_back, on_back, LV_EVENT_CLICKED, NULL);

    s_author_title = ui_label(s_overlay, &ui_font_16, COLOR_TEXT, 164);
    ui_obj_set_pos_raw(s_author_title, 38, 34);
    ui_obj_set_size_raw(s_author_title, 164, 24);
    lv_label_set_long_mode(s_author_title, LV_LABEL_LONG_DOT);
    lv_obj_add_flag(s_author_title, LV_OBJ_FLAG_HIDDEN);

    s_query = lv_textarea_create(s_overlay);
    ui_obj_set_pos_raw(s_query, 45, 34);
    ui_obj_set_size_raw(s_query, 150, 28);
    lv_textarea_set_one_line(s_query, true);
    lv_textarea_set_max_length(s_query, 48);
    lv_textarea_set_placeholder_text(s_query, "Search titles");
    lv_obj_set_style_text_font(s_query, &ui_font_14, 0);
    lv_obj_set_style_text_color(s_query, COLOR_TEXT, 0);
    lv_obj_set_style_text_align(s_query, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_bg_color(s_query, COLOR_CARD, 0);
    lv_obj_set_style_bg_opa(s_query, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(s_query, 3, 0);
    lv_obj_set_style_border_color(s_query, COLOR_ACCENT, LV_PART_CURSOR);
    lv_obj_set_style_border_width(s_query, 0, 0);
    lv_obj_set_style_radius(s_query, 8, 0);
    lv_obj_add_event_cb(s_query, on_query_changed, LV_EVENT_VALUE_CHANGED, NULL);

    s_results = lv_obj_create(s_overlay);
    ui_obj_set_pos_raw(s_results, 20, 66);
    ui_obj_set_size_raw(s_results, 200, 62);
    lv_obj_set_style_bg_opa(s_results, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_results, 0, 0);
    lv_obj_set_style_pad_all(s_results, 0, 0);
    lv_obj_set_style_pad_row(s_results, 2, 0);
    lv_obj_set_flex_flow(s_results, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(s_results, LV_SCROLLBAR_MODE_ACTIVE);
    lv_obj_set_style_bg_color(s_results, COLOR_ACCENT, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(s_results, LV_OPA_COVER, LV_PART_SCROLLBAR);
    lv_obj_add_flag(s_results, LV_OBJ_FLAG_SCROLLABLE);

    s_keyboard = lv_keyboard_create(s_overlay);
    // lv_keyboard_create() aligns to the bottom by default; clear that alignment before setting
    // absolute raw coordinates, or the y offset is added below the display.
    lv_obj_set_align(s_keyboard, LV_ALIGN_TOP_LEFT);
    ui_obj_set_pos_raw(s_keyboard, 0, 130);
    ui_obj_set_size_raw(s_keyboard, 240, 110);
    lv_keyboard_set_mode(s_keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_keyboard_set_textarea(s_keyboard, s_query);
    lv_keyboard_set_popovers(s_keyboard, true);
    lv_obj_set_style_text_font(s_keyboard, &ui_font_14, LV_PART_ITEMS);
    lv_obj_set_style_text_color(s_keyboard, COLOR_TEXT, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(s_keyboard, COLOR_CARD, 0);
    lv_obj_set_style_bg_opa(s_keyboard, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(s_keyboard, COLOR_CARD, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(s_keyboard, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_border_color(s_keyboard, COLOR_MUTED, LV_PART_ITEMS);
    lv_obj_set_style_border_width(s_keyboard, 1, LV_PART_ITEMS);
    lv_obj_set_style_radius(s_keyboard, 4, LV_PART_ITEMS);
    lv_obj_set_style_pad_all(s_keyboard, 2, 0);

    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
}

void search_open(bool by_author)
{
    if (!s_overlay) return;
    s_by_author = by_author;
    s_visible = true;
    s_author_books_view = false;
    lv_label_set_text(s_author_title, "");
    lv_obj_add_flag(s_author_title, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_query, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_keyboard, LV_OBJ_FLAG_HIDDEN);
    ui_obj_set_pos_raw(s_results, 20, 66);
    ui_obj_set_size_raw(s_results, 200, 62);
    lv_textarea_set_placeholder_text(s_query, by_author ? "Search authors" : "Search titles");
    lv_textarea_set_text(s_query, "");
    lv_obj_add_state(s_query, LV_STATE_FOCUSED);
    lv_textarea_set_cursor_pos(s_query, LV_TEXTAREA_CURSOR_LAST);
    lv_keyboard_set_textarea(s_keyboard, s_query);
    lv_obj_remove_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_overlay);
    refresh_results();
}

void search_close(void)
{
    if (!s_overlay) return;
    s_visible = false;
    s_author_books_view = false;
    lv_label_set_text(s_author_title, "");
    lv_keyboard_set_textarea(s_keyboard, NULL);
    lv_obj_remove_state(s_query, LV_STATE_FOCUSED);
    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
}

void search_refresh(void)
{
    if (s_visible) refresh_results();
}

bool search_visible(void)
{
    return s_visible;
}
