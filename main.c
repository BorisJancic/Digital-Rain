#include <curses.h>
#include <limits.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MIN_DELAY 10
#define MAX_DELAY 200
#define MAX_PERIOD 200
#define MIN_PERIOD 20
#define MIN_HEIGHT 4
#define X_SLACK 10
#define Y_SLACK 0
#define KEY_ESC '\x1b'

typedef struct __RGB {
    short red;
    short green;
    short blue;
} RGB;

typedef struct __Drop {
    int x_pos;
    int y_pos;
    int height;
    uint32_t last_wchar;

    struct __Drop* p_prev;
    struct __Drop* p_next;
} Drop;

#define NUM_COLORS 5
#define NUM_SHADES 4
RGB test_shades[NUM_SHADES] = {
    {255, 0, 0},      // red
    {0, 255, 0},    // green
    {0, 0, 255},      // blue
    {255, 255, 0},        // yellow
};
RGB green_shades[NUM_SHADES] = {
    {34, 180, 85},      // green
    {128, 206, 135},    // light green
    {56, 165, 49},      // mid green
    {32, 72, 41},      // dark green
};
RGB blue_shades[NUM_SHADES] = {
    {74, 184, 249},     // bright blue
    {9, 65, 152},       // blue
    {14, 35, 115},      // dark blue
    {9,0,136},          // dark blue
};
RGB red_shades[NUM_SHADES] = {
    {212,0,0},
    {240,57,57},
    {148,0,0},
    {92,16,16},
};
RGB yellow_shades[NUM_SHADES] = {
    {242, 226, 76},
    {189, 171, 8},
    {176, 161, 27},
    {150, 135, 0},
};

int shade_index = 0;
RGB* shades_list[NUM_COLORS] = {
    test_shades,
    green_shades,
    blue_shades,
    red_shades,
    yellow_shades,
};

#define N 54
uint32_t p_utf32_chars[N] = {
    L'ﾊ', L'ﾐ', L'ﾋ', L'ｰ', L'ｳ', L'ｼ', L'ﾅ', L'ﾓ', L'ﾆ', L'ｻ', L'ﾜ',
    L'ﾂ', L'ｵ', L'ﾘ', L'ｱ', L'ﾎ', L'ﾃ', L'ﾏ', L'ｹ', L'ﾒ', L'ｴ', L'ｶ',
    L'ｷ', L'ﾑ', L'ﾕ', L'ﾗ', L'ｾ', L'ﾈ', L'ｽ', L'ﾀ', L'ﾇ', L'ﾍ', L'0',
    L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'Y', L'Z',
    L':', L'.', L'=', L'*', L'+', L'-', L'<', L'>', L'¦', L'|'
};

// check for errors
uint32_t print_rand_white_char(int y, int x);
void print_char_rand_shade(int y, int x, uint32_t utf32_char);
void print_blank_char(int y, int x);

void print_debug_info();
void menu(WINDOW* p_window, Drop** p_p_head);
void set_color_scheme(RGB* p_shades);

void shift_color_scheme(int direction);
void shift_delay(int direction);
void shift_period(int direction);
void shift_height(int direction);

Drop* alloc_and_init_drop(int x, int maximum);
void increment_drop(Drop* p_drop);
void push_front_drop(Drop** p_p_head, Drop* p_drop);
Drop* pop_and_free_drop(Drop** p_p_head, Drop* p_drop);
void free_drop_list(Drop** p_p_head);

bool paused = false;
int delay = 100;
int delay_step = 10;
int period = 50;
int period_step = 5;
double height_fraction = 0.5f;
double height_fraction_step = 0.05f;

char p_utf8_encoded[4];

int count_alloc = 0;
int count_free = 0;
int count_max_allocated = 0;

int main() {
    WINDOW* p_window = NULL;
    Drop* p_head = NULL;
    int input = ERR;

    if (
        strcmp(setlocale(LC_CTYPE, "UTF-8"), "UTF-8") ||
        (p_window = initscr()) == NULL ||
        noecho() == ERR ||
        cbreak() == ERR ||
        start_color() == ERR ||
        init_color(NUM_SHADES + 1, 1000, 1000, 1000) == ERR ||
        init_pair(NUM_SHADES + 1, NUM_SHADES + 1, COLOR_BLACK) == ERR
    ) { exit(-1); }

    if (nodelay(NULL, true) == ERR) { timeout(1); }
    keypad(p_window, TRUE);
    curs_set(0);
    srand(time(NULL));

    set_color_scheme(green_shades);
    menu(p_window, &p_head);

    int maximum = 100;
    int* p_y_margin_arr = malloc(maximum * sizeof(int));
    for (int i = 0; i < maximum; i++) { p_y_margin_arr[i] = INT_MAX; }

    while (1) {
        while ((input = getch()) != ERR || input == KEY_RESIZE || paused) {
            switch (input) {
                case L' ': { paused = !paused; } break;
                case L'M': case L'm': { menu(p_window, &p_head); } break;
                case L'X': case L'x': { set_color_scheme(test_shades); } break;
                case L'R': case L'r': { set_color_scheme(red_shades); } break;
                case L'G': case L'g': { set_color_scheme(green_shades); } break;
                case L'B': case L'b': { set_color_scheme(blue_shades); } break;
                case L'Y': case L'y': { set_color_scheme(yellow_shades); } break;
                case KEY_UP: { shift_delay(-1); } break;
                case KEY_DOWN: { shift_delay(1); } break;
                case KEY_LEFT: { shift_color_scheme(-1); } break;
                case KEY_RIGHT: { shift_color_scheme(1); } break;
                case L'W': case L'w': { shift_period(-1); } break;
                case L'S': case L's': { shift_period(1); } break;
                case L'A': case L'a': { shift_height(-1); } break;
                case L'D': case L'd': { shift_height(1); } break;
                case KEY_ESC: case L'Q': case L'q': { goto __exit; } break;

                case KEY_RESIZE: usleep(delay * 500); break;
                case ERR: { // when paused
                    refresh();
                    usleep(delay * 1000);
                } break;
                default: break;
            }
        }

        int max_x = getmaxx(p_window);
        int max_y = getmaxy(p_window);
        if (max_x > maximum) {
            maximum = max_x;
            free(p_y_margin_arr);
            p_y_margin_arr = malloc(max_x * sizeof(int));
            // for (int i = 0; i < max_x; i++) { p_y_margin_arr[i] = INT_MAX; }
        }
        // int* p_y_margin_arr = malloc(max_x * sizeof(int));
        // if (!p_y_margin_arr) { goto __exit; }
        for (int i = 0; i < max_x; i++) { p_y_margin_arr[i] = INT_MAX; }

        Drop* p_curr = p_head;
        int count_allocated = 0;
        while (p_curr) {
            ++count_allocated;
            if (p_curr->x_pos < max_x) {
                if (p_curr->y_pos - p_curr->height < p_y_margin_arr[p_curr->x_pos]) {
                    p_y_margin_arr[p_curr->x_pos] = p_curr->y_pos - p_curr->height;
                }
                increment_drop(p_curr);
            }

            if (
                    p_curr->x_pos > max_x + X_SLACK ||
                    p_curr->y_pos - p_curr->height > max_y
            ) { p_curr = pop_and_free_drop(&p_head, p_curr); }
            else { p_curr = p_curr->p_next; }
        }
        
        if (count_allocated > count_max_allocated) {
            count_max_allocated = count_allocated;
        }

        for (int i = 0; i < max_x; i++) {
            int rand_n = rand();
            int n = period;
            if (p_y_margin_arr[i] < 5) { continue; }
            if (
                    (rand_n % n == 1) && (rand_n % max_y < p_y_margin_arr[i]) ||
                    false && (rand() % p_y_margin_arr[i]) % 1000 == 0
            ) {
                Drop* p_new = alloc_and_init_drop(i, max_y);
                push_front_drop(&p_head, p_new);
            }
        }

        // free(p_y_margin_arr);
        refresh();
        usleep(delay * 1000);
    }

__exit:
    endwin();
    free(p_y_margin_arr);
    free_drop_list(&p_head);
    print_debug_info();

    return 0;
}

void menu(WINDOW* p_window, Drop** p_p_head) {
    WINDOW* p_copy = dupwin(p_window);
    if (!p_copy) { return; }

    clear();
    nodelay(p_window, false);

    int input = KEY_RESIZE;
    do {
        int i = 0;
        mvwprintw(p_window, i, 0, "Press any key to exit menu"); ++i;
        move(i, 0); clrtoeol(); ++i;

        mvwprintw(p_window, i, 0, "MENU:            M"); ++i;
        mvwprintw(p_window, i, 0, "QUIT:            Q or ESCAPE"); ++i;
        mvwprintw(p_window, i, 0, "Play / Pause:    SPACE"); ++i;
        move(i, 0); clrtoeol(); ++i;

        mvwprintw(p_window, i, 0, "Change Speed:    UP and DOWN"); ++i;
        mvwprintw(p_window, i, 0, "Change Color:    LEFT and RIGHT"); ++i;
        mvwprintw(p_window, i, 0, "Change Length:   A and D"); ++i;
        mvwprintw(p_window, i, 0, "Change Density:  W and S"); ++i;
        move(i, 0); clrtoeol(); ++i;

        mvwprintw(p_window, i, 0, "Color Red:       R"); ++i;
        mvwprintw(p_window, i, 0, "Color Green:     G"); ++i;
        mvwprintw(p_window, i, 0, "Color Blue:      B"); ++i;
        mvwprintw(p_window, i, 0, "Color Yellow:    Y"); ++i;
        move(i, 0); clrtoeol(); ++i;

        refresh();
    } while ((input = getch()) == KEY_RESIZE);
    if (input == L'Q' || input == L'q') {
        free_drop_list(p_p_head);
        delwin(p_copy);
        endwin();
        print_debug_info();
        exit(0);
    }
    nodelay(p_window, true);

    overwrite(p_copy, p_window);
    refresh();
    delwin(p_copy);
}

void print_debug_info() {
    printf("count_alloc: %d\r\n", count_alloc);
    printf("count_free: %d\r\n", count_free);
    printf("Bytes leaked: %ld\r\n", (count_alloc - count_free) * sizeof(Drop));
    printf("Max Drops Allocated: %d\r\n", count_max_allocated);
    printf("Max Bytes Allocated: %ld\r\n", count_max_allocated * sizeof(Drop));
}

Drop* alloc_and_init_drop(int x, int maximum) {
    Drop* p_drop = malloc(sizeof(Drop));
    if (!p_drop) {
        endwin();
        exit(-1);
    }

    maximum = (int)(maximum * height_fraction);
    if (maximum < 6) { maximum = 6; }

    p_drop->x_pos = x;
    p_drop->y_pos = -1;
    p_drop->height = MIN_HEIGHT + rand() % (maximum + 1 - MIN_HEIGHT);
    p_drop->last_wchar = L' ';
    p_drop->p_prev = NULL;
    p_drop->p_next = NULL;

    ++count_alloc;
    return p_drop;
}
void increment_drop(Drop* p_drop) {
    print_blank_char(p_drop->y_pos - p_drop->height, p_drop->x_pos);
    print_char_rand_shade(p_drop->y_pos, p_drop->x_pos, p_drop->last_wchar);
    ++p_drop->y_pos;
    p_drop->last_wchar = print_rand_white_char(p_drop->y_pos, p_drop->x_pos);
}
void push_front_drop(Drop** p_p_head, Drop* p_drop) {
    if (*p_p_head) {
        p_drop->p_next = *p_p_head;
        (*p_p_head)->p_prev = p_drop;
    }
    *p_p_head = p_drop;
}
Drop* pop_and_free_drop(Drop** p_p_head, Drop* p_drop) {
    if (!p_drop  || !*p_p_head) { return NULL; }

    Drop* p_next = p_drop->p_next;
    if (!p_drop->p_prev) {
        *p_p_head = p_drop->p_next;
    } else {
        p_drop->p_prev->p_next = p_drop->p_next;
    }
    if (p_drop->p_next) {
        p_drop->p_next->p_prev = p_drop->p_prev;
    }

    free(p_drop);
    ++count_free;
    return p_next;
}
void free_drop_list(Drop** p_p_head) {
    if (!p_p_head) { return; }

    Drop* p_curr = *p_p_head;
    Drop* p_next = NULL;
    while (p_curr) {
        p_next = p_curr->p_next;
        free(p_curr);
        ++count_free;
        p_curr = p_next;
    }
    *p_p_head = NULL;
}

inline uint32_t print_rand_white_char(int y, int x) {
    uint32_t utf32_char = p_utf32_chars[rand() % N];
    int len = wctomb(p_utf8_encoded, utf32_char);
    attron(COLOR_PAIR(NUM_SHADES + 1)); {
        mvaddnstr(y, x, p_utf8_encoded, len);
    } attroff(COLOR_PAIR(NUM_SHADES + 1));

    return utf32_char;
}
inline void print_char_rand_shade(int y, int x, uint32_t utf32_char) {
    int rand_shade = rand() % NUM_SHADES + 1;
    int len = wctomb(p_utf8_encoded, utf32_char);
    attron(COLOR_PAIR(rand_shade)); {
        mvaddnstr(y, x, p_utf8_encoded, len);
    } attroff(COLOR_PAIR(rand_shade));
}

inline void print_blank_char(int y, int x) { mvaddch(y, x, ' '); }

void set_color_scheme(RGB* p_shades) {
    for (int i = 0; i < NUM_COLORS; i++) {
        if (shades_list[i] == p_shades) { shade_index = i; break; }
    }
    for (int i = 0; i < NUM_SHADES; i++) {
        short red = (1000 * p_shades[i].red) / 255;
        short green = (1000 * p_shades[i].green) / 255;
        short blue = (1000 * p_shades[i].blue) / 255;
        init_color(i + 1, red, green, blue);
        init_pair(i + 1, i + 1, COLOR_BLACK);
    }
}

void shift_color_scheme(int direction) {
    if (direction > 0) {
        shade_index = (shade_index + 1) % NUM_COLORS;
        set_color_scheme(shades_list[shade_index]);
    } else if (direction < 0) {
        shade_index = (shade_index - 1 + NUM_COLORS) % NUM_COLORS;
        set_color_scheme(shades_list[shade_index]);
    }
}

void shift_delay(int direction) {
    if (direction > 0 && (delay + delay_step) <= MAX_DELAY) {
        delay += delay_step;
    } else if (direction < 0 && (delay - delay_step) >= MIN_DELAY) {
        delay -= delay_step;
    }
}

void shift_period(int direction) {
    if (direction > 0 && (period + period_step) <= MAX_PERIOD) {
        period += period_step;
    } else if (direction < 0 && (period - period_step) >= MIN_PERIOD) {
        period -= period_step;
    }
}

void shift_height(int direction) {
    if (direction > 0 && height_fraction < 1.01f) {
        height_fraction += height_fraction_step;
    } else if (direction < 0 && height_fraction > 0.06f) {
        height_fraction -= height_fraction_step;
    }
}



