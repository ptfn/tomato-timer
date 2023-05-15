#include <ncurses.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DELAY 20000
#define WORK  1500
#define BREAK 300

typedef unsigned char   u8;
typedef unsigned short  u16;
typedef unsigned long   u32;

typedef struct Clock {
    struct 
    {
        u16 x;
        u16 y;
        u8  h;
        u8  w;
    } geo;

    struct 
    {
        clock_t start;
        clock_t end;
        bool    state;
    } clock;
  
    u8      color;
    bool    run;
    WINDOW *win;
} Clock;

Clock timer;

const bool number[][15] =
{
    {1,1,1,1,0,1,1,0,1,1,0,1,1,1,1},
    {0,0,1,0,0,1,0,0,1,0,0,1,0,0,1},
    {1,1,1,0,0,1,1,1,1,1,0,0,1,1,1},
    {1,1,1,0,0,1,1,1,1,0,0,1,1,1,1},
    {1,0,1,1,0,1,1,1,1,0,0,1,0,0,1},
    {1,1,1,1,0,0,1,1,1,0,0,1,1,1,1},
    {1,1,1,1,0,0,1,1,1,1,0,1,1,1,1},
    {1,1,1,0,0,1,0,0,1,0,0,1,0,0,1},
    {1,1,1,1,0,1,1,1,1,1,0,1,1,1,1},
    {1,1,1,1,0,1,1,1,1,0,0,1,1,1,1}
};

void clock_move(u16 x, u16 y)
{
    u16 bord_x, bord_y;
    getmaxyx(stdscr, bord_y, bord_x);

    if (x >= 0 && x <= bord_x-34 
        && y >= 0 && y <= bord_y-5) {
        timer.geo.x = x;
        timer.geo.y = y;
        mvwin(timer.win, timer.geo.y, timer.geo.x);
        wrefresh(timer.win);
        clear();
    }
}

void center_clock()
{
    u16 bord_x, bord_y;
    getmaxyx(stdscr, bord_y, bord_x);

    clock_move((bord_x / 2 - (timer.geo.w / 2)), 
               (bord_y  / 2 - (timer.geo.h / 2)));
}

void key_event(void)
{
    struct timespec length = {0, 0};
    u16 i, c;
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);

    switch(c = wgetch(stdscr)) {
        case KEY_UP: case 'k': case 'K':
            clock_move(timer.geo.x, timer.geo.y-1);
            break;

        case KEY_DOWN: case 'j': case 'J':
            clock_move(timer.geo.x, timer.geo.y+1);
            break;

        case KEY_LEFT: case 'h': case 'H':
            clock_move(timer.geo.x-1, timer.geo.y);
            break;

        case KEY_RIGHT: case 'l': case 'L':
            clock_move(timer.geo.x+1, timer.geo.y);
            break;
        
        case 's': case 'S': case 'q': case 'Q':
            timer.run = false;
            break;

        case 'c': case 'C':
            center_clock();
            break;

        case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8':
            i = c - '0';
            timer.color = i; 
            break;

        default:
            pselect(1, &rfds, NULL, NULL, &length, NULL);
    }
    return;
}

void print_char(u16 x, u16 y)
{
    wattron(timer.win, COLOR_PAIR(timer.color));
    mvwaddch(timer.win, y, x, ' ');
    mvwaddch(timer.win, y, x+1, ' ');
    wattroff(timer.win, COLOR_PAIR(timer.color));
}

void draw_column(u16 x, u16 y)
{
    for (u16 i = y; i < 5+y;  i++) {
        if (i-y == 1 || i-y == 3) {
            print_char(x, i);            
        }
    }
}

void draw_number(u8 n, u16 x, u16 y)
{
    for (u16 i = y, iter = 0; i < 5+y;  i++) {
        for (u16 j = x; j < 6+x; j += 2, iter++) {
            if (number[n][iter]) {
               print_char(j, i); 
            }
        }
    }
}

void format(u32 time)
{
    u8 second = time % 60;
    u8 minute = time / 60;

    draw_number(minute / 10, 0, 0);
    draw_number(minute % 10, 8, 0);
    draw_column(16, 0);
    draw_number(second / 10, 20, 0);
    draw_number(second % 10, 28, 0);
    wrefresh(timer.win);
}

void notify(char *message)
{
    char command[50] = "notify-send ";
    strcat(command, message);
    system(command);
}

void draw_clock()
{
    timer.clock.end = clock() - timer.clock.start;
    u32 time = timer.clock.end / CLOCKS_PER_SEC;
    
    if (timer.clock.state) {
        if (BREAK-time <= 0) {
            notify("'Tomato Clock' 'Work time 25 minutes!'");
            format(BREAK - time);
            timer.clock.state = false;
            timer.clock.start = clock();
        } else {
            format(BREAK - time);
        }
    } else {
        if (WORK-time <= 0) {
            notify("'Tomato Clock' 'Break time 5 minutes!'");
            timer.clock.state = true;
            timer.clock.start = clock();
            format(WORK - time);
        } else {
            format(WORK - time);
        }
    }
}

int main()
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, true);
    use_default_colors();
    start_color();
    curs_set(false);
    nodelay(stdscr, true);

    timer.geo.x = timer.geo.y = 0;
    timer.geo.h = 5;
    timer.geo.w = 34;
    timer.clock.start = clock();
    timer.clock.state = false;

    timer.color = 1;
    timer.run = true;
    wresize(timer.win, timer.geo.w, timer.geo.h);

    timer.win = newwin(timer.geo.h,
                       timer.geo.w,
                       timer.geo.x,
                       timer.geo.y);
    box(timer.win, 0, 0);
    wrefresh(timer.win);

    init_pair(1, COLOR_GREEN, COLOR_GREEN);
    init_pair(2, COLOR_BLACK, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_RED);
    init_pair(4, COLOR_YELLOW, COLOR_YELLOW);
    init_pair(5, COLOR_BLUE, COLOR_BLUE);
    init_pair(6, COLOR_MAGENTA, COLOR_MAGENTA);
    init_pair(7, COLOR_CYAN, COLOR_CYAN);
    init_pair(8, COLOR_WHITE, COLOR_WHITE);

    while (timer.run) {
        draw_clock(); 
        key_event();
        werase(timer.win);
    }
    endwin();
    return 0;
}
