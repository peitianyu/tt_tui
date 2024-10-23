#include "core/c_test.h"
#include "core/c_log.h"
#include "core/tui.h"
#include <signal.h>


struct ui_t *g_ui = NULL;

void draw_box1(struct ui_t *ui)
{
    ui_box(dict_get(ui->box_dict, "box1"));
}

void draw_box2(struct ui_t *ui)
{
    ui_box(dict_get(ui->box_dict, "box2"));
}

void draw_text(struct ui_t *ui)
{
    ui_text(dict_get(ui->text_dict, "text1"));    
}

void draw_retext(struct ui_t *ui)
{
    ui_retext(dict_get(ui->text_dict, "text1"), "World");    
}

void exit_handler(int sig)
{
    ui_free(g_ui);
}

void ui_key_exit(struct ui_t *ui) {
    if(ui_kbhit()) {
        if(getchar() == 'q') ui_free(ui);
    }
}

JUST_RUN_TEST(tui_basic, test);
TEST(tui_basic, test)
{
    signal(SIGINT, exit_handler);
    signal(SIGSEGV, exit_handler);
    signal(SIGABRT, exit_handler);

    g_ui = ui_new();
    dict_put(g_ui->box_dict, "box1", &(struct ui_box_t){1, 1, 20, 10, "box1"});
    dict_put(g_ui->box_dict, "box2", &(struct ui_box_t){1, 21, 20, 10, "box2"});
    dict_put(g_ui->text_dict, "text1", &(struct ui_text_t){2, 2, "Hello", TEXT_BLACK});

    list_push(g_ui->event_list, draw_box1);
    list_push(g_ui->event_list, draw_box2);
    list_push(g_ui->event_list, draw_text);
    list_push(g_ui->event_list, draw_retext);

    list_push(g_ui->long_life_event, ui_key_exit);
    
    ui_loop(g_ui);
}

