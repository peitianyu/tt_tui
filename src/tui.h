#ifndef __TUI_H__
#define __TUI_H__

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>

/* __LIST_H__ */
typedef struct __ListNode {
    void *elem;
    struct __ListNode *next, *prev;
} ListNode;

typedef struct {
    int len;
    ListNode *head, *tail;
} List;

typedef struct {
    ListNode *ptr;
} Iter;

#define EMPTY_LIST ((List){.len = 0, .head = NULL, .tail = NULL})

static inline List *make_list(void) {
    List *r = malloc(sizeof(List));
    r->len = 0;
    r->head = r->tail = NULL;
    return r;
}
static inline void *make_node(void *elem) {
    ListNode *r = malloc(sizeof(ListNode));
    r->elem = elem;
    r->next = NULL;
    r->prev = NULL;
    return r;
}
static inline void list_push(List *list, void *elem) {
    ListNode *node = make_node(elem);
    if (!list->head) list->head = node;
    else {
        list->tail->next = node;
        node->prev = list->tail;
    }
    list->tail = node;
    list->len++;
}
static inline void *list_pop(List *list) {
    if (!list->head) return NULL;
    ListNode *tail = list->tail;
    void *r = tail->elem;
    list->tail = tail->prev;
    if (list->tail) list->tail->next = NULL;
    else list->head = NULL;
    free(tail);
    list->len--;
    return r;
}
static inline int list_len(List *list) {return list->len; }
static void list_unshift(List *list, void *elem) {
    ListNode *node = make_node(elem);
    node->next = list->head;
    if (list->head) list->head->prev = node;
    list->head = node;
    if (!list->tail) list->tail = node;
    list->len++;
}
static inline Iter list_iter(void *ptr) {return (Iter){ .ptr = ((List *) ptr)->head, };}
static inline bool iter_end(const Iter iter) {return !iter.ptr;}
static inline void *iter_next(Iter *iter) {
    if (!iter->ptr) return NULL;
    void *r = iter->ptr->elem;
    iter->ptr = iter->ptr->next;
    return r;
}
static inline List *list_reverse(List *list) {
    List *r = make_list();
    for (Iter i = list_iter(list); !iter_end(i);) list_unshift(r, iter_next(&i));
    return r;
}
static inline bool list_empty(List *list) { return !list->head; }
#define list_safe_next(node) ((node) ? (node)->next : NULL)
#define list_for_each_safe(node, tmp, list)                           \
    for ((node) = (list)->head, (tmp) = list_safe_next(node); (node); \
         (node) = (tmp), (tmp) = list_safe_next(node))
static inline void list_free(List *list) {
    ListNode *node, *tmp;
    list_for_each_safe (node, tmp, list) {
        free(node->elem);
        free(node);
    }
}
static inline ListNode *list_id(List *list, int id) {
    int i = 0;
    ListNode *node, *tmp;
    list_for_each_safe (node, tmp, list) {
        if (i++ == id) return node;
    }
    return NULL;
}
/* __LIST_H__ */

/* __DICT_H__ */
typedef struct Dict {
    List *list;
    struct Dict *parent;
} Dict;

#define EMPTY_DICT ((Dict){&EMPTY_LIST, NULL})

typedef struct {
    char *key;
    void *val;
} DictEntry;

static inline void *make_dict(void *parent) {
    Dict *r = malloc(sizeof(Dict));
    r->list = make_list();
    r->parent = parent;
    return r;
}
static inline void *dict_get(Dict *dict, char *key) {
    for (; dict; dict = dict->parent) {
        for (Iter i = list_iter(dict->list); !iter_end(i);) {
            DictEntry *e = iter_next(&i);
            if (!strcmp(key, e->key)) return e->val;
        }
    }
    return NULL;
}
static inline void dict_put(Dict *dict, char *key, void *val) {
    DictEntry *e = malloc(sizeof(DictEntry));
    e->key = key;
    e->val = val;
    list_push(dict->list, e);
}
/* __DICT_H__ */

/* __TUI_H__ */
struct ui_box_t {
    uint row, col;
    uint width, height;
    char *box_name;
};

#define TEXT_BLUE      "\033[44m"
#define TEXT_BLACK     "\033[40m"
#define TEXT_NONE      "\033[0m"
struct ui_text_t {
    uint row, col;
    char *text;
    char* ground_color; 

    int cursor;
};

struct ui_input_t {
    uint row, col;
    uint width, height;
    char *text;
    char* ground_color; 
};

struct ui_dropdown_t {
    uint row, col;
    uint width;

    char *text;
    List *list; // list of char*
    int cursor;  
};

struct ui_t {
    Dict *text_dict;
    Dict *box_dict;
    Dict *input_dict;
    Dict *dropdown_dict;
    List *event_list;
    List *long_life_event;
};

#define ui_loop(u, ...) u->event_list = list_reverse(u->event_list); ui_hide_cursor(); ui_clear_terminal(); ui_enable_mode(); do { \
    while(!list_empty(u->event_list)) {\
        void (*event_func)(struct ui_t *) = list_pop(u->event_list);\
        if(event_func) event_func(u); }\
    ListNode *node, *tmp;                               \
    list_for_each_safe (node, tmp, u->long_life_event) {\
        void (*event_func)(struct ui_t *) = node->elem;\
        event_func(u); }\
        usleep(2000);\
} while(1);
static inline void ui_hide_cursor() { printf("\033[?25l"); }
static inline void ui_show_cursor() { printf("\033[?25h"); }
static inline void ui_clear_terminal(void) { printf("\033[2J"); }
static inline void ui_enable_mode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
static inline void ui_disable_mode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static inline struct ui_t *ui_new(void) {
    struct ui_t *u = malloc(sizeof(struct ui_t));
    u->box_dict = make_dict(NULL);
    u->text_dict = make_dict(NULL);
    u->input_dict = make_dict(NULL);
    u->dropdown_dict = make_dict(NULL);
    u->event_list = make_list();
    u->long_life_event = make_list();

    return u;
}

static inline void ui_free(struct ui_t *ui) {
    free(ui);
    ui_show_cursor();
    ui_disable_mode();
    ui_clear_terminal();
    exit(1);
}

// ui组件
static inline void ui_print(int row, int col, char *text, ...){
    va_list args;
    va_start(args, text);
    char buf[1024];
    vsnprintf(buf, 1024, text, args);
    va_end(args);
    printf("\033[%d;%dH%s", row, col, buf);
}
////////////////////box组件
static inline void ui_box(struct ui_t *u, char *box_id)
{
    struct ui_box_t *box = dict_get(u->box_dict, box_id);
    if(!box) return;
    int len = strlen(box->box_name);
    box->width = (len > box->width - 2) ? len + 2 : box->width;

    ui_print(box->row, box->col, "┌%s", box->box_name);
    for (int i = 0; i < box->width - 2 - len; ++i) printf("─");
    printf("┐\n");

    for (uint i = 0; i < box->height - 2; ++i) {
        ui_print(box->row + 1 + i, box->col, "│");
        ui_print(box->row + 1 + i, box->col + box->width - 1, "│\n");
    }

    ui_print(box->row + box->height - 1, box->col, "└");
    for (int i = 0; i < box->width - 2; ++i) printf("─");
    ui_print(box->row + box->height - 1, box->col + box->width - 1, "┘\n");
}
////////////////////text组件
static inline void ui_text(struct ui_t *u, char *text_id)
{
    struct ui_text_t *text = dict_get(u->text_dict, text_id);
    if(!text) return;
    if(text->cursor == -1) {
        printf("\033[%d;%dH", text->row, text->col); 
        printf("%s%s\033[0m", text->ground_color, text->text);
    }else {
        printf("\033[%d;%dH%s", text->row, text->col, text->ground_color); 
        for (int i = 0; i < text->cursor; ++i) printf("%c", text->text[i]);
        printf("%s%c%s", TEXT_BLUE, text->text[text->cursor], text->ground_color);
        for (int i = text->cursor+1; i < strlen(text->text); ++i) printf("%c", text->text[i]);
        printf("\033[0m");
    }
}
static inline void ui_text_cursor(struct ui_t *u, char *text_id, int d_cursor) {
    struct ui_text_t *text = dict_get(u->text_dict, text_id);
    if(!text) return;
    text->cursor += d_cursor;
    int text_len = (int)strlen(text->text);
    text->cursor = (text->cursor < 0) ? 0 : (text->cursor > text_len-1 ? text_len-1 : text->cursor);
    ui_text(u, text_id);
}
static inline void ui_retext(struct ui_t *u, char *text_id, char *new)
{
    struct ui_text_t *text = dict_get(u->text_dict, text_id);
    if(!text) return;
    printf("\033[%d;%dH", text->row, text->col);
    printf("%*c", (int)strlen(text->text), ' ');
    text->text = new;
    ui_text(u, text_id);
}
////////////////////input_field组件
static inline void ui_input_field(struct ui_t *u, char *input_field_id)
{
    struct ui_input_t *input_field = dict_get(u->input_dict, input_field_id);
    if(!input_field) return;

    ui_print(input_field->row, input_field->col, "┌");
    for (int i = 0; i < input_field->width - 2; ++i) printf("─");
    printf("┐\n");

    for (uint i = 0; i < input_field->height - 2; ++i) {
        ui_print(input_field->row + 1 + i, input_field->col, "│");
        ui_print(input_field->row + 1 + i, input_field->col + input_field->width - 1, "│\n");
    }

    ui_print(input_field->row + input_field->height - 1, input_field->col, "└");
    for (int i = 0; i < input_field->width - 2; ++i) printf("─");
    ui_print(input_field->row + input_field->height - 1, input_field->col + input_field->width - 1, "┘\n");

    int str_len = strlen(input_field->text);
    if(str_len == 0) return;
    printf("\033[%d;%dH", input_field->row+1, input_field->col+1);
    printf("%*c", input_field->width-2, ' ');
    if(str_len < input_field->width - 2) {
        printf("\033[%d;%dH", input_field->row+1, input_field->col+1);
        printf("%s%s\033[0m", input_field->ground_color, input_field->text);
    }else {
        printf("\033[%d;%dH%s", input_field->row+1, input_field->col+1, input_field->ground_color); 
        printf("%.*s\033[0m", input_field->width - 2, input_field->text + input_field->width - 2);
    }
}

static inline void ui_input_field_add_ch(struct ui_t *u, char *input_field_id, char ch) {
    struct ui_input_t *input_field = dict_get(u->input_dict, input_field_id);
    if(!input_field) return;
    char *str = (char*)malloc(strlen(input_field->text) + 2);
    strcpy(str, input_field->text);
    str[strlen(input_field->text)] = ch;
    str[strlen(input_field->text) + 1] = '\0';
    input_field->text = str;
    printf("\033[%d;%dH", input_field->row+1, input_field->col+1);
    printf("%*c", input_field->width-2, ' ');
    ui_input_field(u, input_field_id);
}

static inline void ui_input_field_del_ch(struct ui_t *u, char *input_field_id) {
    struct ui_input_t *input_field = dict_get(u->input_dict, input_field_id);
    if(!input_field) return;
    printf("\033[%d;%dH", input_field->row+1, input_field->col+1);
    printf("%*c", input_field->width-2, ' ');
    input_field->text[strlen(input_field->text) - 1] = '\0';
    ui_input_field(u, input_field_id);
}
////////////////////下拉框组件
static inline void ui_dropdowm_print(char *text, int width, char ch)
{
    int str_len = strlen(text);
    if(str_len < width - 2) {
        printf("%c\033[4m%s", ch, text);
        printf("%*c\033[0m", (uint)(width - 2 - strlen(text)), ' ');
    }else {
        printf("%c\033[4m%.*s\033[0m", ch, width - 2, text);
    }
}
static inline void ui_dropdown_close(struct ui_t *u, char *dropdown_id) {
    struct ui_dropdown_t *dropdown = dict_get(u->dropdown_dict, dropdown_id);
    if(!dropdown) return;
    printf("\033[%d;%dH", dropdown->row, dropdown->col);
    ui_dropdowm_print(dropdown->text, dropdown->width, '>');
}

static inline void ui_dropdown_open(struct ui_t *u, char *dropdown_id, int d_cursor) {
    struct ui_dropdown_t *dropdown = dict_get(u->dropdown_dict, dropdown_id);
    if(!dropdown) return;

    printf("\033[%d;%dH", dropdown->row, dropdown->col);
    dropdown->cursor += d_cursor;
    dropdown->cursor = dropdown->cursor < 0 ? 0 : (dropdown->cursor > list_len(dropdown->list)-1) ? list_len(dropdown->list)-1 : dropdown->cursor;
    dropdown->text = list_id(dropdown->list, dropdown->cursor)->elem;
    ui_dropdowm_print(dropdown->text, dropdown->width, '>');
    
    for(int i = 0; i < list_len(dropdown->list); ++i) {
        printf("\033[%d;%dH", dropdown->row+1+i, dropdown->col);
        if(i == dropdown->cursor) printf("\033[7m");
        ui_dropdowm_print(list_id(dropdown->list, i)->elem, dropdown->width, ' ');
    }
}
////////////////////key组件
static inline int ui_key_get_ch()  {
    int oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    int ch = getchar();
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    return ch;
}
enum {RET=10, ESC=27, BACKSPACE=127, ARROW_LEFT = 0x3e8, ARROW_RIGHT, ARROW_UP, ARROW_DOWN};
static inline int ui_read_key() {
    int ch = ui_key_get_ch();
    if(ch == '\x1b' && ui_key_get_ch() == '[') {
        switch(ui_key_get_ch()) {
            case 'A': return ARROW_UP;
            case 'B': return ARROW_DOWN;
            case 'C': return ARROW_RIGHT;
            case 'D': return ARROW_LEFT;
        }
    }
    return ch;
}


#endif // __TUI_H__