#include "tui.h"
struct ui_t *ui_new(void)
{
    ui_clear_terminal();
    struct ui_t *u = malloc(sizeof(struct ui_t));

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &(u->ws));

    tcgetattr(STDIN_FILENO, &u->tio);

    u->box_dict = make_dict(NULL);
    u->text_dict = make_dict(NULL);
    u->event_list = make_list();
    u->long_life_event = make_list();

    return u;
}

void ui_free(struct ui_t *ui)
{
    free(ui);
    tcsetattr(STDIN_FILENO, TCSANOW, &(ui->tio));
    ui_clear_terminal();
    exit(1);
}

void ui_clear_terminal(void) {
    printf("\033[2J");
}

void ui_box(struct ui_box_t *box) {
    int len = strlen(box->box_name);
    
    box->width = (len > box->width - 2) ? len + 2 : box->width;

    printf("\033[%d;%dH", box->row, box->col); 
    printf("┌%s", box->box_name);
    for (int i = 0; i < box->width - 2 - len; ++i) printf("─");
    printf("┐\n");

    for (uint i = 0; i < box->height - 2; ++i) {
        printf("\033[%d;%dH", box->row + 1 + i, box->col);
        printf("│");
        for (int j = 0; j < box->width - 2; ++j)printf(" ");
        printf("│\n");
    }

    printf("\033[%d;%dH", box->row + box->height - 1, box->col); 
    printf("└");
    for (int i = 0; i < box->width - 2; ++i) printf("─");
    printf("┘\n");
}

void ui_text(struct ui_text_t *text)
{
    printf("\033[%d;%dH", text->row, text->col); 
    printf("%s%s\033[0m", text->ground_color, text->text);
}

void ui_retext(struct ui_text_t *text, char *new)
{
    printf("\033[%d;%dH", text->row, text->col);
    printf("%*c", (int)strlen(text->text), ' ');
    text->text = new;
    ui_text(text);
}

bool ui_kbhit(void) 
{
    struct termios oldt, newt;
    int oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    int ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if(ch != EOF) {
        ungetc(ch, stdin); // push back the character
        return 1;
    }

    return 0;
}