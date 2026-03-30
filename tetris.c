#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/time.h>
#endif

#define MAX_WIDTH 20
#define MAX_HEIGHT 40
#define PIECE_BLOCKS 4
#define SIDE_BOX_W 4
#define SIDE_BOX_H 2

typedef struct { int x, y; } Block;
typedef struct { int type; int rotation; int x; int y; } Piece;

enum {
    COLOR_DEFAULT = 39,
    COLOR_DARKGRAY = 90,
    COLOR_RED = 31,
    COLOR_GREEN = 32,
    COLOR_YELLOW = 33,
    COLOR_BLUE = 34,
    COLOR_MAGENTA = 35,
    COLOR_CYAN = 36,
    COLOR_GRAY = 37,
    COLOR_ORANGE = 91
};

typedef struct {
    int width;
    int height;
    int fall_time_ms;
    int enable_drop_projection;
    int score;
    int high_score;
    int running;
    int board[MAX_HEIGHT][MAX_WIDTH];
} GameState;

static GameState g = {10, 20, 800, 1, 0, 0, 1, {{0}}};

static const Block SHAPES[7][4][4] = {
    /* I */
    {
        {{0,1},{1,1},{2,1},{3,1}},
        {{2,0},{2,1},{2,2},{2,3}},
        {{0,2},{1,2},{2,2},{3,2}},
        {{1,0},{1,1},{1,2},{1,3}}
    },
    /* J */
    {
        {{0,0},{0,1},{1,1},{2,1}},
        {{1,0},{2,0},{1,1},{1,2}},
        {{0,1},{1,1},{2,1},{2,2}},
        {{1,0},{1,1},{0,2},{1,2}}
    },
    /* L */
    {
        {{2,0},{0,1},{1,1},{2,1}},
        {{1,0},{1,1},{1,2},{2,2}},
        {{0,1},{1,1},{2,1},{0,2}},
        {{0,0},{1,0},{1,1},{1,2}}
    },
    /* O */
    {
        {{1,0},{2,0},{1,1},{2,1}},
        {{1,0},{2,0},{1,1},{2,1}},
        {{1,0},{2,0},{1,1},{2,1}},
        {{1,0},{2,0},{1,1},{2,1}}
    },
    /* S */
    {
        {{1,0},{2,0},{0,1},{1,1}},
        {{1,0},{1,1},{2,1},{2,2}},
        {{1,1},{2,1},{0,2},{1,2}},
        {{0,0},{0,1},{1,1},{1,2}}
    },
    /* T */
    {
        {{1,0},{0,1},{1,1},{2,1}},
        {{1,0},{1,1},{2,1},{1,2}},
        {{0,1},{1,1},{2,1},{1,2}},
        {{1,0},{0,1},{1,1},{1,2}}
    },
    /* Z */
    {
        {{0,0},{1,0},{1,1},{2,1}},
        {{2,0},{1,1},{2,1},{1,2}},
        {{0,1},{1,1},{1,2},{2,2}},
        {{1,0},{0,1},{1,1},{0,2}}
    }
};

static const int PIECE_COLORS[7] = {
    COLOR_CYAN, COLOR_BLUE, COLOR_ORANGE,
    COLOR_YELLOW, COLOR_GREEN, COLOR_MAGENTA, COLOR_RED
};

#ifdef _WIN32
static void sleep_ms(int ms) { Sleep(ms); }
static int key_pressed(void) { return _kbhit(); }
static int read_key(void) { return _getch(); }
#else
static struct termios original_termios;
static void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}
static void enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &original_termios);
    atexit(disable_raw_mode);
    struct termios raw = original_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
static void sleep_ms(int ms) { struct timeval tv; tv.tv_sec = ms / 1000; tv.tv_usec = (ms % 1000) * 1000; select(0, NULL, NULL, NULL, &tv); }
static int key_pressed(void) {
    fd_set set;
    struct timeval tv = {0, 0};
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    return select(STDIN_FILENO + 1, &set, NULL, NULL, &tv) > 0;
}
static int read_key(void) {
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) == 1) return c;
    return -1;
}
#endif

static void clear_screen(void) {
    printf("\x1b[2J\x1b[H");
}

static void hide_cursor(void) { printf("\x1b[?25l"); fflush(stdout); }
static void show_cursor(void) { printf("\x1b[?25h\x1b[0m\n"); fflush(stdout); }

static void gotoxy(int x, int y) {
    printf("\x1b[%d;%dH", y + 1, x + 1);
}

static void set_fg(int color) { printf("\x1b[%dm", color); }
static void set_bg_from_fg(int color) {
    int bg = 40;
    switch (color) {
        case COLOR_RED: bg = 41; break;
        case COLOR_GREEN: bg = 42; break;
        case COLOR_YELLOW: bg = 43; break;
        case COLOR_BLUE: bg = 44; break;
        case COLOR_MAGENTA: bg = 45; break;
        case COLOR_CYAN: bg = 46; break;
        case COLOR_GRAY: bg = 47; break;
        case COLOR_DARKGRAY: bg = 100; break;
        case COLOR_ORANGE: bg = 101; break;
        default: bg = 40; break;
    }
    printf("\x1b[%dm", bg);
}

static void reset_style(void) { printf("\x1b[0m"); }

static int score_for_lines(int lines) {
    if (lines <= 0) return 0;
    switch (lines) {
        case 1: return 1000;
        case 2: return 6000;
        case 3: return 16000;
        default: return 32000;
    }
}

static void piece_blocks(Piece p, Block out[4]) {
    for (int i = 0; i < 4; ++i) {
        out[i].x = p.x + SHAPES[p.type][p.rotation][i].x;
        out[i].y = p.y + SHAPES[p.type][p.rotation][i].y;
    }
}

static int collision(const GameState *gs, Piece p) {
    Block b[4];
    piece_blocks(p, b);
    for (int i = 0; i < 4; ++i) {
        if (b[i].x < 0 || b[i].x >= gs->width || b[i].y >= gs->height) return 1;
        if (b[i].y >= 0 && gs->board[b[i].y][b[i].x] != 0) return 1;
    }
    return 0;
}

static Piece random_piece(const GameState *gs) {
    Piece p;
    p.type = rand() % 7;
    p.rotation = 0;
    p.x = gs->width / 2 - 2;
    p.y = 0;
    return p;
}

static void lock_piece(GameState *gs, Piece p) {
    Block b[4];
    piece_blocks(p, b);
    for (int i = 0; i < 4; ++i) {
        if (b[i].y >= 0 && b[i].y < gs->height && b[i].x >= 0 && b[i].x < gs->width) {
            gs->board[b[i].y][b[i].x] = p.type + 1;
        }
    }
}

static int clear_lines(GameState *gs) {
    int cleared = 0;
    for (int y = gs->height - 1; y >= 0; --y) {
        int full = 1;
        for (int x = 0; x < gs->width; ++x) {
            if (gs->board[y][x] == 0) { full = 0; break; }
        }
        if (full) {
            ++cleared;
            for (int yy = y; yy > 0; --yy) {
                for (int x = 0; x < gs->width; ++x) gs->board[yy][x] = gs->board[yy - 1][x];
            }
            for (int x = 0; x < gs->width; ++x) gs->board[0][x] = 0;
            ++y;
        }
    }
    gs->score += score_for_lines(cleared);
    if (gs->score > gs->high_score) gs->high_score = gs->score;
    return cleared;
}

static int game_over(const GameState *gs) {
    for (int x = 0; x < gs->width; ++x) {
        if (gs->board[0][x] || gs->board[1][x]) return 1;
    }
    return 0;
}

static Piece project_drop(const GameState *gs, Piece p) {
    Piece d = p;
    while (1) {
        Piece next = d;
        next.y++;
        if (collision(gs, next)) break;
        d = next;
    }
    return d;
}

static int block_in_piece(Piece p, int x, int y) {
    Block b[4];
    piece_blocks(p, b);
    for (int i = 0; i < 4; ++i) if (b[i].x == x && b[i].y == y) return 1;
    return 0;
}

static void draw_box(int x, int y, int w, int h) {
    gotoxy(x, y); printf("+");
    for (int i = 0; i < w * 2; ++i) printf("-");
    printf("+");
    for (int row = 0; row < h; ++row) {
        gotoxy(x, y + 1 + row); printf("|");
        gotoxy(x + w * 2 + 1, y + 1 + row); printf("|");
    }
    gotoxy(x, y + h + 1); printf("+");
    for (int i = 0; i < w * 2; ++i) printf("-");
    printf("+");
}

static void draw_cell(int sx, int sy, int color, const char *text) {
    gotoxy(sx, sy);
    if (color) {
        set_bg_from_fg(color);
        printf("  ");
        reset_style();
    } else {
        printf("%s", text);
    }
}

static void render(const GameState *gs, Piece current, Piece next) {
    clear_screen();
    set_fg(COLOR_GRAY);
    draw_box(0, 0, gs->width, gs->height);
    draw_box(gs->width * 2 + 3, 0, SIDE_BOX_W, SIDE_BOX_H);
    draw_box(gs->width * 2 + 3, 4, SIDE_BOX_W, SIDE_BOX_H);

    set_fg(COLOR_RED);
    gotoxy(gs->width * 2 + 4, 1); printf("!");
    gotoxy(gs->width * 2 + 4, 2); printf("!");

    set_fg(COLOR_CYAN);
    gotoxy(gs->width * 2 + 6, 0); printf("NEXT");
    gotoxy(gs->width * 2 + 6, 4); printf("SCORE");

    Piece shadow = project_drop(gs, current);
    for (int y = 0; y < gs->height; ++y) {
        for (int x = 0; x < gs->width; ++x) {
            int screen_x = 1 + x * 2;
            int screen_y = 1 + y;
            if (gs->board[y][x]) {
                draw_cell(screen_x, screen_y, PIECE_COLORS[gs->board[y][x] - 1], NULL);
            } else if (block_in_piece(current, x, y)) {
                draw_cell(screen_x, screen_y, PIECE_COLORS[current.type], NULL);
            } else if (gs->enable_drop_projection && block_in_piece(shadow, x, y)) {
                gotoxy(screen_x, screen_y);
                set_fg(PIECE_COLORS[current.type]);
                printf("[]");
                reset_style();
            } else {
                set_fg(COLOR_DARKGRAY);
                gotoxy(screen_x, screen_y);
                printf("[]");
            }
        }
    }

    for (int y = 0; y < SIDE_BOX_H; ++y) {
        for (int x = 0; x < SIDE_BOX_W; ++x) {
            gotoxy(gs->width * 2 + 4 + x * 2, 1 + y);
            printf("[]");
            gotoxy(gs->width * 2 + 4 + x * 2, 5 + y);
            printf("[]");
        }
    }

    Block n[4];
    piece_blocks((Piece){next.type, 0, 0, 0}, n);
    for (int i = 0; i < 4; ++i) {
        draw_cell(gs->width * 2 + 4 + n[i].x * 2, 1 + n[i].y, PIECE_COLORS[next.type], NULL);
    }

    reset_style();
    gotoxy(gs->width * 2 + 4, 5); printf("%8d", gs->score);
    gotoxy(0, gs->height + 3);
    printf("Controls: Left/Right - move | Up - drop | Down - soft drop | A/D - rotate | X - exit");
    fflush(stdout);
}

static int read_input_key(void) {
#ifdef _WIN32
    int ch = read_key();
    if (ch == 0 || ch == 224) {
        int ext = read_key();
        switch (ext) {
            case 72: return 'U';
            case 80: return 'D';
            case 75: return 'L';
            case 77: return 'R';
            default: return 0;
        }
    }
    return ch;
#else
    int c = read_key();
    if (c == 27 && key_pressed()) {
        int c2 = read_key();
        int c3 = read_key();
        if (c2 == '[') {
            switch (c3) {
                case 'A': return 'U';
                case 'B': return 'D';
                case 'C': return 'R';
                case 'D': return 'L';
            }
        }
        return 0;
    }
    return c;
#endif
}

static void try_move(Piece *p, int dx, int dy) {
    Piece n = *p;
    n.x += dx;
    n.y += dy;
    if (!collision(&g, n)) *p = n;
}

static void try_rotate(Piece *p, int dir) {
    Piece n = *p;
    n.rotation = (n.rotation + dir + 4) % 4;
    if (!collision(&g, n)) {
        *p = n;
        return;
    }
    int kicks[] = {-1, 1, -2, 2};
    for (int i = 0; i < 4; ++i) {
        n = *p;
        n.rotation = (n.rotation + dir + 4) % 4;
        n.x += kicks[i];
        if (!collision(&g, n)) { *p = n; return; }
    }
}

static void reset_board(GameState *gs) {
    memset(gs->board, 0, sizeof(gs->board));
    gs->score = 0;
}

static void wait_key(void) {
    while (!key_pressed()) sleep_ms(20);
    (void)read_input_key();
}

static void play_game(void) {
    reset_board(&g);
    Piece current = random_piece(&g);
    Piece next = random_piece(&g);
    long long last_fall = 0;

    while (1) {
        render(&g, current, next);

        while (key_pressed()) {
            int key = read_input_key();
            if (key == 'x' || key == 'X') return;
            if (key == 'L') try_move(&current, -1, 0);
            else if (key == 'R') try_move(&current, 1, 0);
            else if (key == 'D') {
                Piece n = current; n.y++;
                if (!collision(&g, n)) current = n;
                else {
                    lock_piece(&g, current);
                    clear_lines(&g);
                    if (game_over(&g)) goto end_game;
                    current = next;
                    next = random_piece(&g);
                }
            } else if (key == 'U') {
                current = project_drop(&g, current);
                lock_piece(&g, current);
                clear_lines(&g);
                if (game_over(&g)) goto end_game;
                current = next;
                next = random_piece(&g);
            } else if (key == 'a' || key == 'A') try_rotate(&current, -1);
            else if (key == 'd' || key == 'D') try_rotate(&current, 1);
            render(&g, current, next);
        }

#ifdef _WIN32
        long long now = GetTickCount64();
#else
        struct timeval tv_now;
        gettimeofday(&tv_now, NULL);
        long long now = (long long)tv_now.tv_sec * 1000LL + tv_now.tv_usec / 1000LL;
#endif
        if (last_fall == 0) last_fall = now;
        if (now - last_fall >= g.fall_time_ms) {
            Piece n = current;
            n.y++;
            if (!collision(&g, n)) {
                current = n;
            } else {
                lock_piece(&g, current);
                clear_lines(&g);
                if (game_over(&g)) break;
                current = next;
                next = random_piece(&g);
                if (collision(&g, current)) break;
            }
            last_fall = now;
        }
        sleep_ms(10);
    }

end_game:
    if (g.score > g.high_score) g.high_score = g.score;
    clear_screen();
    printf("GAME OVER\n\nScore: %d\nSession High Score: %d\n\nPress any key to return to menu...", g.score, g.high_score);
    fflush(stdout);
    wait_key();
}

static void settings_menu(void) {
    clear_screen();
    printf("SETTINGS\n\n");
    printf("Current width  (4-%d): %d\n", MAX_WIDTH, g.width);
    printf("Current height (8-%d): %d\n", MAX_HEIGHT, g.height);
    printf("Current fall time (ms): %d\n", g.fall_time_ms);
    printf("Drop projection (1=on, 0=off): %d\n\n", g.enable_drop_projection);
    printf("Enter new values, or press Enter to keep current.\n\n");

#ifndef _WIN32
    disable_raw_mode();
#endif
    char buf[64];
    printf("Width: "); fflush(stdout);
    if (fgets(buf, sizeof(buf), stdin) && buf[0] != '\n') {
        int v = atoi(buf); if (v >= 4 && v <= MAX_WIDTH) g.width = v;
    }
    printf("Height: "); fflush(stdout);
    if (fgets(buf, sizeof(buf), stdin) && buf[0] != '\n') {
        int v = atoi(buf); if (v >= 8 && v <= MAX_HEIGHT) g.height = v;
    }
    printf("Fall time (ms): "); fflush(stdout);
    if (fgets(buf, sizeof(buf), stdin) && buf[0] != '\n') {
        int v = atoi(buf); if (v >= 50) g.fall_time_ms = v;
    }
    printf("Drop projection (1/0): "); fflush(stdout);
    if (fgets(buf, sizeof(buf), stdin) && buf[0] != '\n') {
        int v = atoi(buf); g.enable_drop_projection = v ? 1 : 0;
    }
    printf("\nSaved. Press Enter to continue...");
    fflush(stdout);
    fgets(buf, sizeof(buf), stdin);
#ifndef _WIN32
    enable_raw_mode();
#endif
}

static void help_menu(void) {
    clear_screen();
    printf("HELP\n\n");
    printf("Controls:\n");
    printf("  Left/Right arrows - move piece\n");
    printf("  A / D             - rotate left / right\n");
    printf("  Down arrow        - soft drop\n");
    printf("  Up arrow          - instant drop\n");
    printf("  X                 - exit to menu\n\n");
    printf("Scoring:\n");
    printf("  1 line  = 1000\n");
    printf("  2 lines = 6000\n");
    printf("  3 lines = 16000\n");
    printf("  4 lines = 32000\n\n");
    printf("Press any key to return...");
    fflush(stdout);
    wait_key();
}

static void title_screen(void) {
    while (g.running) {
        clear_screen();
        printf("\n");
        printf("  _____ _____ _____ ____  ___ ____  \n");
        printf(" |_   _| ____|_   _|  _ \\|_ _/ ___| \n");
        printf("   | | |  _|   | | | |_) || | \\___ \\\n");
        printf("   | | | |___  | | |  _ < | |  ___) |\n");
        printf("   |_| |_____| |_| |_| \\_\\___|____/ \n\n");
        printf("Session High Score: %d\n\n", g.high_score);
        printf("1. Play\n");
        printf("2. Settings\n");
        printf("3. Help\n");
        printf("4. Exit\n\n");
        printf("Select: ");
        fflush(stdout);

        int ch = 0;
        while (!key_pressed()) sleep_ms(10);
        ch = read_input_key();

        if (ch == '1') play_game();
        else if (ch == '2') settings_menu();
        else if (ch == '3') help_menu();
        else if (ch == '4' || ch == 'x' || ch == 'X') g.running = 0;
    }
}

int main(void) {
    srand((unsigned int)time(NULL));
#ifndef _WIN32
    enable_raw_mode();
#endif
    hide_cursor();
    title_screen();
    show_cursor();
#ifndef _WIN32
    disable_raw_mode();
#endif
    return 0;
}
