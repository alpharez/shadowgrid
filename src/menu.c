#include "menu.h"
#include "render.h"

#include <string.h>

void menu_draw_box(WINDOW *win, const char *title)
{
    wattron(win, COLOR_PAIR(COL_MENU_TITLE));
    box(win, 0, 0);

    if (title) {
        int w = getmaxx(win);
        int tlen = (int)strlen(title);
        int tx = (w - tlen - 2) / 2;
        if (tx < 1) tx = 1;
        mvwprintw(win, 0, tx, " %s ", title);
    }

    wattroff(win, COLOR_PAIR(COL_MENU_TITLE));
}

void menu_draw_list(WINDOW *win, int y, int x,
                    const char **items, int count, int selected)
{
    for (int i = 0; i < count; i++) {
        if (i == selected) {
            wattron(win, COLOR_PAIR(COL_MENU_SEL) | A_BOLD);
            mvwprintw(win, y + i, x, "> %s", items[i]);
            wattroff(win, COLOR_PAIR(COL_MENU_SEL) | A_BOLD);
        } else {
            wattron(win, COLOR_PAIR(COL_MENU_DIM));
            mvwprintw(win, y + i, x, "  %s", items[i]);
            wattroff(win, COLOR_PAIR(COL_MENU_DIM));
        }
    }
}
