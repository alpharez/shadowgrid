#pragma once

#include <ncurses.h>

/* Draw a box with an optional title centred in the top border.
 * title may be NULL for a plain box. */
void menu_draw_box(WINDOW *win, const char *title);

/* Draw a vertical list of items inside win starting at (y, x).
 * The item at index `selected` is highlighted. */
void menu_draw_list(WINDOW *win, int y, int x,
                    const char **items, int count, int selected);
