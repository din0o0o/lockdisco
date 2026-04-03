// lockdisco — LED pattern sequencer for Num/Caps/Scroll Lock keys
// Compile: windres lockdisco.rc -O coff -o lockdisco.res
//          gcc lockdisco.c lockdisco.res -o lockdisco.exe

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>


// ── Definitions ──────────────────────────────────────────────

#define NUM_LEDS   3
#define MAX_BEATS  32
#define BEATS_MIN  2
#define BPM_MIN    30
#define BPM_MAX    300
#define BPM_STEP   5

// Screen layout
#define ROW_TITLE   1
#define ROW_HEADER  2
#define ROW_GRID    3
#define ROW_STATUS  7
#define COL_BPM     7
#define COL_BEATS   19
#define COL_GRID    9
#define CELL_W      2

// Colors (palette index: fg = low nibble, bg = high nibble)
#define CLR_NORMAL  0x07
#define CLR_TITLE   0x0A   // dino green foreground (palette 10)
#define CLR_PLAY    0xA0   // dino green background, black text
#define CLR_EDIT    0x0E
#define CLR_CURSOR  0x70

const char *led_names[NUM_LEDS] = { "NUM ", "CAPS", "SCR " };
const int   led_vkeys[NUM_LEDS] = { VK_NUMLOCK, VK_CAPITAL, VK_SCROLL };

int pattern[NUM_LEDS][MAX_BEATS] = {
    { 1, 0, 0, 1, 0, 0, 1, 0 },
    { 0, 0, 1, 0, 0, 1, 0, 0 },
    { 0, 1, 0, 0, 1, 0, 0, 0 },
};

int num_beats = 8;
int bpm = 120;
int initial_led[NUM_LEDS];

HANDLE hcon;


// ── Console helpers ──────────────────────────────────────────

void setup_palette(void) {
    CONSOLE_SCREEN_BUFFER_INFOEX info = { sizeof(info) };
    GetConsoleScreenBufferInfoEx(hcon, &info);
    SMALL_RECT win = info.srWindow;
    info.ColorTable[10] = 0x003AF3A6;   // dino green #a6f33a (COLORREF = 0x00BBGGRR)
    SetConsoleScreenBufferInfoEx(hcon, &info);
    SetConsoleWindowInfo(hcon, TRUE, &win);
}

void cur(int row, int col) {
    SetConsoleCursorPosition(hcon, (COORD){ (SHORT)col, (SHORT)row });
}

void color(WORD attr) {
    SetConsoleTextAttribute(hcon, attr);
}

void cls(void) {
    DWORD n;
    COORD origin = {0, 0};
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(hcon, &info);
    DWORD size = info.dwSize.X * info.dwSize.Y;
    FillConsoleOutputCharacter(hcon, ' ', size, origin, &n);
    FillConsoleOutputAttribute(hcon, CLR_NORMAL, size, origin, &n);
    cur(0, 0);
}


// ── LED control ──────────────────────────────────────────────

int led_state(int led) {
    return GetKeyState(led_vkeys[led]) & 1;
}

void led_toggle(int led) {
    INPUT inputs[2] = {0};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = led_vkeys[led];
    inputs[0].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = led_vkeys[led];
    inputs[1].ki.dwFlags = KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP;
    SendInput(2, inputs, sizeof(INPUT));
}

void led_set(int led, int on) {
    if (led_state(led) != on)
        led_toggle(led);
}

void leds_off(void) {
    for (int i = 0; i < NUM_LEDS; i++)
        led_set(i, 0);
}

void leds_restore(void) {
    for (int i = 0; i < NUM_LEDS; i++)
        led_set(i, initial_led[i]);
}


// ── Display ──────────────────────────────────────────────────

void draw_cell(int led, int beat, WORD clr) {
    cur(ROW_GRID + led, COL_GRID + beat * CELL_W);
    color(clr);
    printf("%c ", pattern[led][beat] ? 'X' : '.');
}

void draw_bpm(WORD clr) {
    cur(ROW_HEADER, COL_BPM);
    color(clr);
    printf("%-3d", bpm);
    color(CLR_NORMAL);
}

void draw_num_beats(WORD clr) {
    cur(ROW_HEADER, COL_BEATS);
    color(clr);
    printf("%-2d", num_beats);
    color(CLR_NORMAL);
}

void draw_status(const char *msg) {
    cur(ROW_STATUS, 0);
    color(CLR_NORMAL);
    printf("  %-70s", msg);
}

void draw_column(int beat, WORD clr) {
    for (int led = 0; led < NUM_LEDS; led++)
        draw_cell(led, beat, clr);
    color(CLR_NORMAL);
}

void draw_grid(void) {
    color(CLR_NORMAL);
    for (int r = ROW_GRID; r <= ROW_GRID + NUM_LEDS - 1; r++) {
        cur(r, 0);
        printf("%-80s", "");
    }

    for (int led = 0; led < NUM_LEDS; led++) {
        cur(ROW_GRID + led, 2);
        printf("%s [ ", led_names[led]);
        for (int b = 0; b < num_beats; b++)
            printf("%c ", pattern[led][b] ? 'X' : '.');
        printf("]");
    }
}

void draw(void) {
    cls();

    color(CLR_TITLE);
    cur(ROW_TITLE, 2);
    printf("Lockdisco");
    color(CLR_NORMAL);

    cur(ROW_HEADER, 2);
    printf("BPM: ");
    draw_bpm(CLR_NORMAL);
    cur(ROW_HEADER, 12);
    printf("Beats: ");
    draw_num_beats(CLR_NORMAL);

    draw_grid();
    draw_status("[Space]Play  [E]dit  [Q]uit");
}


// ── Playback ─────────────────────────────────────────────────

int play(void) {
    int beat = 0;
    draw_status("[Space]Stop  [Q]uit");

    while (1) {
        int ms = 60000 / bpm;

        draw_column(beat, CLR_PLAY);
        for (int led = 0; led < NUM_LEDS; led++)
            led_set(led, pattern[led][beat]);

        for (int elapsed = 0; elapsed < ms; elapsed += 10) {
            if (_kbhit()) {
                int ch = _getch();
                if (ch == 0 || ch == 0xE0) { _getch(); continue; }

                draw_column(beat, CLR_NORMAL);
                leds_off();

                if (ch == 'q' || ch == 'Q') return 1;
                draw_status("[Space]Play  [E]dit  [Q]uit");
                return 0;
            }
            Sleep(10);
        }

        draw_column(beat, CLR_NORMAL);
        beat = (beat + 1) % num_beats;
    }
}


// ── Edit mode ────────────────────────────────────────────────

void edit_recolor(int erow, int ecol) {
    draw_bpm(CLR_EDIT);
    draw_num_beats(CLR_EDIT);
    for (int led = 0; led < NUM_LEDS; led++)
        for (int b = 0; b < num_beats; b++)
            draw_cell(led, b, CLR_EDIT);

    if (erow >= 0)
        draw_cell(erow, ecol, CLR_CURSOR);
    else if (ecol == 0)
        draw_bpm(CLR_CURSOR);
    else
        draw_num_beats(CLR_CURSOR);
}

void edit(void) {
    int row = 0, col = 0;

    draw_status("[Arrows]Move  [Space]Toggle  [+/-]Value  [E]Done");
    edit_recolor(row, col);

    while (1) {
        int ch = _getch();

        // Arrow keys
        if (ch == 0 || ch == 0xE0) {
            int arrow = _getch();

            if (row >= 0)
                draw_cell(row, col, CLR_EDIT);
            else if (col == 0)
                draw_bpm(CLR_EDIT);
            else
                draw_num_beats(CLR_EDIT);

            if (row >= 0) {
                switch (arrow) {
                    case 72: if (row > 0) row--; else { row = -1; col = 0; } break;
                    case 80: if (row < NUM_LEDS - 1) row++; break;
                    case 75: if (col > 0) col--; break;
                    case 77: if (col < num_beats - 1) col++; break;
                }
            } else {
                switch (arrow) {
                    case 80: row = 0; col = 0; break;
                    case 75: col = 0; break;
                    case 77: col = 1; break;
                }
            }

            if (row >= 0)
                draw_cell(row, col, CLR_CURSOR);
            else if (col == 0)
                draw_bpm(CLR_CURSOR);
            else
                draw_num_beats(CLR_CURSOR);

            continue;
        }

        switch (ch) {
            case 'e': case 'E':
                draw_bpm(CLR_NORMAL);
                draw_num_beats(CLR_NORMAL);
                for (int led = 0; led < NUM_LEDS; led++)
                    for (int b = 0; b < num_beats; b++)
                        draw_cell(led, b, CLR_NORMAL);
                draw_status("[Space]Play  [E]dit  [Q]uit");
                return;

            case ' ': case 13:
                if (row >= 0) {
                    pattern[row][col] ^= 1;
                    draw_cell(row, col, CLR_CURSOR);
                }
                break;

            case '+': case '=':
                if (row < 0 && col == 0) {
                    if (bpm + BPM_STEP <= BPM_MAX) bpm += BPM_STEP;
                    else bpm = BPM_MAX;
                    draw_bpm(CLR_CURSOR);
                }
                if (row < 0 && col == 1 && num_beats < MAX_BEATS) {
                    num_beats++;
                    draw_grid();
                    draw_num_beats(CLR_NORMAL);
                    edit_recolor(row, col);
                }
                break;

            case '-': case '_':
                if (row < 0 && col == 0) {
                    if (bpm - BPM_STEP >= BPM_MIN) bpm -= BPM_STEP;
                    else bpm = BPM_MIN;
                    draw_bpm(CLR_CURSOR);
                }
                if (row < 0 && col == 1 && num_beats > BEATS_MIN) {
                    num_beats--;
                    draw_grid();
                    draw_num_beats(CLR_NORMAL);
                    edit_recolor(row, col);
                }
                break;
        }
    }
}


// ── Main ─────────────────────────────────────────────────────

int main(void) {
    hcon = GetStdHandle(STD_OUTPUT_HANDLE);
    setup_palette();

    CONSOLE_CURSOR_INFO ci = { 1, FALSE };
    SetConsoleCursorInfo(hcon, &ci);

    for (int i = 0; i < NUM_LEDS; i++)
        initial_led[i] = led_state(i);

    draw();

    while (1) {
        int ch = _getch();
        if (ch == 0 || ch == 0xE0) { _getch(); continue; }

        switch (ch) {
            case ' ':
                if (play()) goto quit;
                break;
            case 'e': case 'E':
                edit();
                break;
            case 'q': case 'Q':
                goto quit;
        }
    }

quit:
    leds_restore();
    color(CLR_NORMAL);
    cur(ROW_STATUS + 1, 0);
    return 0;
}
