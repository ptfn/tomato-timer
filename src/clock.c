#include <ncurses.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#define MAX_PATH 200
#define MAX_TEXT 100

typedef struct {
    struct {
        uint16_t x, y, h, w;
    } geo;

    struct {
        time_t      start, end;
        time_t      start_all, end_all;
        uint16_t    time, time_all;
        bool        state;
    } clock;
  
    uint8_t color;
    bool    run;
    WINDOW *win;
} Clock;

typedef struct {
    sqlite3_stmt *stmt;
    sqlite3 *db;
    char *err;
    int rc;
} Sql;


typedef struct {
    uint16_t work, chill, overwork;
} State;

void load_db(bool choice, char *name_db);
void exec_db(void);
void stop_clock(void);
void move_clock(uint16_t x, uint16_t y);
void center_clock(void);
void key_event(void);
void draw_column(uint16_t x, uint16_t y);
void draw_number(uint16_t n, uint16_t x, uint16_t y);
void draw_clock(void);
void draw_time(void);
void notify(const char *message, char *icon);
void time_clock(uint16_t *timer, time_t *start, time_t *end, bool operation);
void assign_state_time(bool state, uint16_t timer_time);
void generate_path(char *path);
void init(void);

static char work_text[MAX_TEXT], break_text[MAX_TEXT], path_icon[MAX_PATH], work_icon[MAX_PATH], break_icon[MAX_PATH];
static State state = {1500, 300, 7200};
static Clock timer;
static Sql worker;

static const bool number[10][15] =
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

void load_db(bool choice, char *name_db)
{
    worker.err = 0;
    worker.rc = sqlite3_open(name_db, &worker.db);

    if (worker.rc != SQLITE_OK) {
        sqlite3_close(worker.db);
        timer.run = false;
    }

    if (!(choice)) {
        char *sql = "DROP TABLE IF EXISTS task;"
                    "CREATE TABLE timer(id INTEGER PRIMARY KEY AUTOINCREMENT, time INTEGER, time_date REAL);";
        worker.rc = sqlite3_exec(worker.db, sql, 0, 0, &worker.err);

        if (worker.rc != SQLITE_OK) {
            sqlite3_free(worker.err);
            sqlite3_close(worker.db);
            timer.run = false;
        }
    }
}

void exec_db(void)
{
    char *sql = calloc(200, sizeof(char));

    sprintf(sql, "INSERT INTO timer (time, time_date) VALUES (%d, date('now'));", state.work);
    worker.rc = sqlite3_exec(worker.db, sql, 0, 0, &worker.err);
                
    if (worker.rc != SQLITE_OK) {
        sqlite3_free(worker.err);
        sqlite3_close(worker.db);
        timer.run = false;
    }

}

void stop_clock(void)
{ 
    struct timespec length = {1, 0};
    bool run = true;
    uint16_t c;
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);

    mvwprintw(timer.win, 5, 34/2-2, "%s", "STOP");
    wrefresh(timer.win);
    
    while (run) {
        c = wgetch(stdscr);

        switch (c) {
            case 's': case 'S':
                run = false;
                break;
            
            case 'q': case 'Q':
                timer.run = false;
                run = false;
                break;

            default:
                pselect(1, &rfds, NULL, NULL, &length, NULL);

        }    
    }

    timer.clock.start = time(NULL);
    timer.clock.start_all = time(NULL);
}

void move_clock(uint16_t x, uint16_t y)
{
    uint16_t bord_x, bord_y;
    getmaxyx(stdscr, bord_y, bord_x);

    if (x >= 0 && x < bord_x-34 &&
        y >= 0 && y < bord_y-5) {
        timer.geo.x = x;
        timer.geo.y = y;
        werase(stdscr);
        mvwin(timer.win, timer.geo.y, timer.geo.x);
        wrefresh(timer.win);
    }
}

void center_clock(void)
{
    uint16_t bord_x, bord_y;
    getmaxyx(stdscr, bord_y, bord_x);
    move_clock((bord_x / 2 - (timer.geo.w / 2)), 
               (bord_y  / 2 - (timer.geo.h / 2)));
}        

void key_event(void)
{
    struct timespec length = {1, 0};
    uint16_t i, c;
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);

    switch(c = wgetch(stdscr)) {
        case KEY_UP: case 'k': case 'K':
            move_clock(timer.geo.x, timer.geo.y-1);
            break;

        case KEY_DOWN: case 'j': case 'J':
            move_clock(timer.geo.x, timer.geo.y+1);
            break;

        case KEY_LEFT: case 'h': case 'H':
            move_clock(timer.geo.x-1, timer.geo.y);
            break;

        case KEY_RIGHT: case 'l': case 'L':
            move_clock(timer.geo.x+1, timer.geo.y);
            break;
        
        case 'r': case 'R':
            timer.clock.time = timer.clock.state ? state.chill : state.work;
            break;

        case 'q': case 'Q':
            timer.run = false;
            break;

        case 'c': case 'C':
            center_clock();
            break;

        case 's': case 'S':
            stop_clock();
            break;

        case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8':
            i = c - '0';
            timer.color = i; 
            break;

        default:
            pselect(1, &rfds, NULL, NULL, &length, NULL);
    }
}

void draw_column(uint16_t x, uint16_t y)
{
    for (uint16_t i = y; i < 5+y;  i++)
        if (i-y == 1 || i-y == 3)
            mvwaddstr(timer.win, i, x, "  ");
}

void draw_number(uint16_t n, uint16_t x, uint16_t y)
{
    for (uint16_t i = y, iter = 0; i < 5+y;  i++)
        for (uint16_t j = x; j < 6+x; j += 2, iter++)
            if (number[n][iter])
                mvwaddstr(timer.win, i, j, "  ");
}

void draw_clock(void)
{
    uint8_t second = timer.clock.time % 60;
    uint8_t minute = timer.clock.time / 60;

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

void draw_time(void)
{
    uint8_t minute = (timer.clock.time_all / 60) % 60;
    uint8_t second = timer.clock.time_all % 60;
    uint8_t hours = timer.clock.time_all / (3600);

    mvwprintw(timer.win, 6, 34/2-4, "%d%d:%d%d:%d%d", hours / 10, hours % 10
                                                    , minute / 10, minute % 10
                                                    , second / 10, second % 10);
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

void time_clock(uint16_t *timer, time_t *start, time_t *end, bool operation)
{
    *end = time(NULL);
    uint16_t diff = (int)difftime(*end, *start);
    *timer = operation ? *timer - diff : *timer + diff;
    // *start = time(NULL);
    *start = *end;

}

void assign_state_time(bool state, uint16_t timer_time)
{
    timer.clock.state = state;
    timer.clock.time = timer_time;
    timer.clock.start = time(NULL);
}

void state_clock(char *work_icon, char *break_icon)
{
    if (state.overwork <= timer.clock.time_all) {
        timer.run = false;
    } else if (timer.clock.state && timer.clock.time <= 0) {
        notify(work_text, work_icon);
        assign_state_time(false, state.work);
    } else if (!(timer.clock.state) && timer.clock.time <= 0) {
        exec_db();
        notify(break_text, break_icon); 
        assign_state_time(true,state.chill);
    }
}

void generate_path(char *path)
{
    strcat(work_icon, path);
    strcat(break_icon, path);

    strcat(work_icon,  "/img/work.svg");
    strcat(break_icon, "/img/break.svg");
}

void init(void)
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
    timer.geo.h = 7;
    timer.geo.w = 34;
    
    timer.clock.state = false;
    timer.clock.start = time(NULL);
    timer.clock.start_all = time(NULL);
    timer.clock.time = state.work;
    timer.clock.time_all = 0;

    timer.color = 1;
    timer.run = true;

    sprintf(work_text, "'Work time %d minutes!'", state.work/60);
    sprintf(break_text, "'Break time %d minutes!'", state.chill/60);

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

}

int main(int argc, char **argv)
{   
    int c;

    while ((c = getopt(argc, argv, "w:b:o:p:n:l:")) != -1) {
        switch (c) {
            case 'w':
                state.work = atoi(optarg)*60;
                break;

            case 'b':
                state.chill = atoi(optarg)*60;
                break;

            case 'o':
                state.overwork = atoi(optarg)*60;
                break;

            case 'p':
                sprintf(path_icon, "%s", optarg);
                generate_path(path_icon);
                break;

            case 'n':
                load_db(0, optarg);
                break;

            case 'l':
                load_db(1, optarg);
                break;
        }
    }

    init();

    while (timer.run) {
        time_clock(&timer.clock.time, &timer.clock.start, &timer.clock.end, 1);
        time_clock(&timer.clock.time_all, &timer.clock.start_all, &timer.clock.end_all, 0);
        draw_clock();
        draw_time();
        state_clock(work_icon, break_icon);
        key_event();
    }

    delwin(timer.win);
    endwin();
    return 0;
}
