#include <ncurses.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define WORK        1500
#define BREAK       300
#define PATH        "/home/ptfn/Project/tomato-timer"
#define MAX_PATH    200

typedef unsigned char   u8;
typedef unsigned short  u16;
typedef unsigned long   u32;

typedef struct Clock {
    struct {
        u16 x, y, h, w;
    } geo;

    struct {
        time_t  start, end;
        u16     time;
        bool    state;
    } clock;
  
    u8      color;
    bool    run;
    WINDOW *win;
} Clock;

Clock timer;

const char *work_text = "'Work time 25 minutes!'"; 
const char *break_text = "'Break time 5 minutes!'"; 

const bool number[10][15] =
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

void stop(void)
{
    WINDOW *win;
    uint16_t ch, yMax, xMax;
    bool run= true;
    
    getmaxyx(stdscr, yMax, xMax); 
    win = newwin(5, xMax/2, yMax/2-(5/2), xMax/2-(xMax/2)/2); 
    xMax = getmaxx(win);

    box(win, 0, 0);            
    wrefresh(win);

    mvwprintw(win, 2, xMax/2-2, "%s", "STOP");
    
    while (run) {
        ch = wgetch(win);

        switch (ch) {
            case 's': case 'S':
                run= false;
                break;
            
            case 'q': case 'Q':
                timer.run = false;
                run = false;
                break;
 
            default:
                break;
        }    
    }

    delwin(win);
    wclear(stdscr);
}

void clock_move(u16 x, u16 y)
{
    u16 bord_x, bord_y;
    getmaxyx(stdscr, bord_y, bord_x);

    if (x >= 0 && x <= bord_x-34 
        && y >= 0 && y <= bord_y-5) {
        timer.geo.x = x;
        timer.geo.y = y;
        werase(stdscr);
        mvwin(timer.win, timer.geo.y, timer.geo.x);
        wrefresh(timer.win);
    }
}

void center_clock(void)
{
    u16 bord_x, bord_y;
    getmaxyx(stdscr, bord_y, bord_x);
    clock_move((bord_x / 2 - (timer.geo.w / 2)), 
               (bord_y  / 2 - (timer.geo.h / 2)));
}        

void key_event(void)
{
    struct timespec length = {1, 0};
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
        
        case 'q': case 'Q':
            timer.run = false;
            break;

        case 'c': case 'C':
            center_clock();
            break;

        case 's': case 'S':
            stop();
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

void draw_column(u16 x, u16 y)
{
    for (u16 i = y; i < 5+y;  i++) {
        if (i-y == 1 || i-y == 3) {
            mvwaddstr(timer.win, i, x, "  ");
        }
    }
}

void draw_number(u8 n, u16 x, u16 y)
{
    for (u16 i = y, iter = 0; i < 5+y;  i++) {
        for (u16 j = x; j < 6+x; j += 2, iter++) {
            if (number[n][iter]) {
                mvwaddstr(timer.win, i, j, "  ");
            }
        }
    }
}

void draw_clock(u32 time)
{
    u8 second = time % 60;
    u8 minute = time / 60;

    werase(timer.win);
    wattron(timer.win, COLOR_PAIR(timer.color));
    draw_number(minute / 10, 0, 0);
    draw_number(minute % 10, 8, 0);
    draw_column(16, 0);
    draw_number(second / 10, 20, 0);
    draw_number(second % 10, 28, 0);
    wattroff(timer.win, COLOR_PAIR(timer.color));
    wrefresh(timer.win);
}

void notify(const char *message, char *icon)
{
    char command[200] = "notify-send 'Tomato Clock' ";
    char *arg = " --icon='";
    strcat(command, message);
    strcat(command, arg);
    strcat(command, icon);
    strcat(command, "'");
    system(command);
}

void time_clock(void)
{
   timer.clock.end = time(NULL);
    u16 diff = (int)difftime(timer.clock.end, timer.clock.start);
    timer.clock.time = (timer.clock.state ? BREAK : WORK) - diff;
}

void state(char *work_icon, char *break_icon)
{
    if (timer.clock.state && timer.clock.time <= 0) {
        notify(work_text, work_icon);
        timer.clock.state = false;
        timer.clock.start = time(NULL); 
    } else if (!(timer.clock.state) && timer.clock.time <= 0) {
        notify(break_text, break_icon); 
        timer.clock.state = true;
        timer.clock.start = time(NULL); 
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
    timer.clock.start = time(NULL);
    timer.clock.state = false;

    timer.color = 1;
    timer.run = true;
    wresize(timer.win, timer.geo.w, timer.geo.h);

    timer.win = newwin(timer.geo.h,
                       timer.geo.w,
                       timer.geo.x,
                       timer.geo.y);
    
    wrefresh(timer.win);

    init_pair(1, COLOR_GREEN, COLOR_GREEN);
    init_pair(2, COLOR_BLACK, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_RED);
    init_pair(4, COLOR_YELLOW, COLOR_YELLOW);
    init_pair(5, COLOR_BLUE, COLOR_BLUE);
    init_pair(6, COLOR_MAGENTA, COLOR_MAGENTA);
    init_pair(7, COLOR_CYAN, COLOR_CYAN);
    init_pair(8, COLOR_WHITE, COLOR_WHITE);

    char work_icon[MAX_PATH], break_icon[MAX_PATH];

    strcat(work_icon, PATH);
    strcat(break_icon, PATH);

    strcat(work_icon,  "/img/work.svg");
    strcat(break_icon, "/img/break.svg");

    while (timer.run) {
        time_clock();
        draw_clock(timer.clock.time);
        state(work_icon, break_icon);
        key_event();
    }

    delwin(timer.win);
    endwin();
    return 0;
}
