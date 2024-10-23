#ifndef __TUI_H__
#define __TUI_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "list.h"
#include "dict.h"

struct ui_box_t {
    uint row, col;
    uint width, height;
    char *box_name;
};

#define TEXT_BLACK     "\033[40m"
#define TEXT_NONE      "\033[0m"
struct ui_text_t {
    uint row, col;
    char *text;
    char* ground_color;
};

struct ui_t
{
    struct winsize ws;
    struct termios tio;

    Dict *text_dict;
    Dict *box_dict;
    List *event_list;
    List *long_life_event;
};

#define ui_loop(u, ...) u->event_list = list_reverse(u->event_list);do { \
    while(!list_empty(u->event_list)) {\
        void (*event_func)(struct ui_t *) = list_pop(u->event_list);\
        if(event_func) event_func(u); }\
    ListNode *node, *tmp;                               \
    list_for_each_safe (node, tmp, u->long_life_event) {\
        void (*event_func)(struct ui_t *) = node->elem;\
        event_func(u); }\
} while(1);

struct ui_t *ui_new(void);
void ui_free(struct ui_t *ui);

void ui_clear_terminal(void);
void ui_box(struct ui_box_t *box);
void ui_text(struct ui_text_t *text);
void ui_retext(struct ui_text_t *text, char *new);
bool ui_kbhit(void);

#endif // __TUI_H__