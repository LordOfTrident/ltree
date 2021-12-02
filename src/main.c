/*
 *  ltree - Merry christmas 2021 :)
 *  Version 1.0.7
 *  Github: https://github.com/LordOfTrident/ltree
 */

#include <stdio.h> // printf, fprintf, fflush
#include <stdlib.h> // malloc, realloc, free, rand
#include <unistd.h> // usleep
#include <time.h> // usleep
#include <string.h> // strcmp

#include <ncurses.h> // Terminal i/o

#include <stdbool.h> // bool, true, false
#include <stdint.h> // uint8_t, uint16_t, uint32_t, uint64_t,
                    // int8_t, int16_t, int32_t, int64_t
#include <stddef.h> // size_t

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef size_t usize;

// Terminal integer type for size/position
typedef u16 uti;
typedef s16 sti; // i made it 16 bit because i doubt anyones terminal
                 // width/height will be bigger than 65535 chars
                 // (the max xfce size i can get is 632x141)
typedef u8 color;

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 7

#define TREE_WIDTH  23
#define TREE_HEIGHT 14

#define TREE_IDX_STAR 0
#define TREE_IDX_STEM 10
#define TREE_IDX_TEXT 12

#define TREE_TOP_START_XIDX 11

#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define eflush()     fflush(stderr)

// Default terminal background and foreground colors
#define COLOR_B_DEFAULT -1
#define COLOR_F_DEFAULT -1

// Color pair macros
#define CLRID_DEFAULT 0
#define CLRID_GREEN   1
#define CLRID_RED     2
#define CLRID_YELLOW  3
#define CLRID_BLUE    4
#define CLRID_MAGENTA 5
#define CLRID_CYAN    6
#define CLRID_WHITE   7

// Error system
#define ERR_FILE __FILE__
#define ERR_FUNC (char*)__func__
#define ERR_LINE __LINE__

#define ERR_WATCH __err_line__ = ERR_LINE
#define ERR_WATCH_LINE __err_line__
#define ERR_WATCH_INIT usize __err_line__ = 0

#define ERR_SET_G_ERROR(p_why) \
	g_error.why  = p_why; \
	g_error.file = ERR_FILE; \
	g_error.func = ERR_FUNC; \
	g_error.line = ERR_WATCH_LINE; \
	g_error.hpnd = true;

typedef struct {
	char *why;
	char *file;
	char *func;
	usize line;
	bool hpnd;
} t_error;

t_error g_error = {
	NULL,  // why
	NULL,  // func
	NULL,  // file
	0,     // line
	false, // hpnd
};

// Baubles
typedef struct {
	uti x;
	uti y;
	color clr;
} t_bbl;

typedef struct {
	t_bbl *buf;
	usize len;
} t_bbls;

t_bbls bbls = {
	NULL, // buf
	0     // len
};

// Snowflakes
typedef struct {
	sti x;
	sti y;
} t_flake;

typedef struct {
	t_flake *buf;
	usize len;
} t_flakes;

t_flakes flakes = {
	NULL, // buf
	0     // len
};

// Screen buffer
typedef struct {
	char ch;
	color clr;
} t_px;

typedef struct {
	uti size_x;
	uti size_y;
	uti size;
	t_px *buf;
	color clr;
} t_scr;

t_scr scr = {
	0,            // size_x
	0,            // size_y
	0,            // size
	NULL,         // buf
	CLRID_DEFAULT // clr
};

bool running = false;
usize tick = 0;
const char tree[TREE_HEIGHT][TREE_WIDTH + 1] = { // include the NULL terminator
	"           &           ",
	"          ***          ",
	"         *****         ",
	"        *******        ",
	"       *********       ",
	"      ***********      ",
	"     *************     ",
	"    ***************    ",
	"   *****************   ",
	"  *******************  ",
	"          wWw          ",
	"          wWw          ",
	"    Merry Christmas    ",
	"And lots of fun coding!",
};
sti tree_pos_x;
sti tree_pos_y;

void error(void) {
	endwin();
	eprintf(
		"LTREE fatal error:\n"
		"  why: %s\n"
		"  file: %s\n"
		"  func: %s\n"
		"  line: %lu\n",
		g_error.why,
		g_error.file,
		g_error.func,
		(unsigned long)g_error.line
	);

	eflush();
	abort();
};

// scr functions
void scr_setsize(t_scr *p_scr, uti p_size_x, uti p_size_y) {
	p_scr->size_x = p_size_x;
	p_scr->size_y = p_size_y;
	p_scr->size   = p_size_x * p_size_y;
};

void scr_new(t_scr *p_scr, uti p_size_x, uti p_size_y) {
	ERR_WATCH_INIT;
	scr_setsize(p_scr, p_size_x, p_size_y);

	p_scr->buf = (t_px*)malloc(sizeof(t_px) * p_scr->size); ERR_WATCH;
	if (p_scr->buf == NULL) {
		ERR_SET_G_ERROR("malloc fail");
	};
};

void scr_free(t_scr *p_scr) {
	ERR_WATCH_INIT; ERR_WATCH;
	if (p_scr->buf == NULL) {
		ERR_SET_G_ERROR("p_scr->buf is NULL");
		return;
	};

	free(p_scr->buf);
	p_scr->buf = NULL;
	p_scr->size_x = 0;
	p_scr->size_y = 0;
};

void scr_resize(t_scr *p_scr, uti p_size_x, uti p_size_y) {
	ERR_WATCH_INIT; ERR_WATCH;
	if (p_scr->buf == NULL) {
		ERR_SET_G_ERROR("p_scr->buf is NULL");
		return;
	};

	usize prev_size_x = p_scr->size_x;
	usize prev_size_y = p_scr->size_y;
	usize prev_size   = prev_size_x * prev_size_y;
	scr_setsize(p_scr, p_size_x, p_size_y);

	void *tmp = realloc(p_scr->buf, sizeof(t_px) * p_scr->size); ERR_WATCH;
	if (tmp == NULL) {
		free(p_scr->buf);
		ERR_SET_G_ERROR("realloc fail");
		return;
	};

	p_scr->buf = (t_px*)tmp;
	if (prev_size < p_scr->size) {
		for (usize i = prev_size; i < p_scr->size; ++ i) {
			p_scr->buf[i].ch  = ' ';
			p_scr->buf[i].clr = CLRID_DEFAULT;
		};
	};
};

void scr_clear(t_scr *p_scr) {
	ERR_WATCH_INIT; ERR_WATCH;
	if (p_scr->buf == NULL) {
		ERR_SET_G_ERROR("p_scr->buf is NULL");
		return;
	};

	for (usize i = 0; i < p_scr->size; ++ i) {
		p_scr->buf[i].ch  = ' ';
		p_scr->buf[i].clr = CLRID_DEFAULT;
	};
};

void scr_draw(t_scr *p_scr) {
	ERR_WATCH_INIT; ERR_WATCH;
	if (p_scr->buf == NULL) {
		ERR_SET_G_ERROR("p_scr->buf is NULL");
		return;
	};

	move(0, 0);
	for (usize i = 0; i < p_scr->size; ++ i) {
		attron(COLOR_PAIR(p_scr->buf[i].clr));
		addch(p_scr->buf[i].ch);
		attroff(COLOR_PAIR(p_scr->buf[i].clr));

		p_scr->buf[i].ch  = ' ';
		p_scr->buf[i].clr = CLRID_DEFAULT;
	};

	refresh();
};

void scr_setcharat(t_scr *p_scr, char p_ch, sti p_x, sti p_y) {
	ERR_WATCH_INIT; ERR_WATCH;
	if (p_scr->buf == NULL) {
		ERR_SET_G_ERROR("p_scr->buf is NULL");
		return;
	};

	if (p_y < 0 || p_y >= p_scr->size_y || p_x < 0 || p_x >= p_scr->size_x)
		return;

	uti pos = p_y * p_scr->size_x + p_x;
	p_scr->buf[pos].ch  = p_ch;
	p_scr->buf[pos].clr = p_scr->clr;
};

// Baubles
void bbls_new(t_bbls *p_bbls) {
	ERR_WATCH_INIT;
	p_bbls->buf = (t_bbl*)malloc(sizeof(t_bbl)); ERR_WATCH;
	if (p_bbls->buf == NULL) {
		ERR_SET_G_ERROR("malloc fail");
		error();
	};
	p_bbls->len = 0; // Yes, im sure i want to set the len to 0 while
	                 // there is one object in the list

	for (usize i = 0; i < TREE_IDX_STEM; ++ i) {
		bool started = false;
		usize cnt = 0;
		for (usize j = 0; j < TREE_WIDTH; ++ j) {
			if (started) {
				if (tree[i][j + 1] != '*') {
					started = false;
					continue;
				};

				if (cnt >= j)
					continue;
				else if (rand() % 3 == 1) {
					++ cnt; ++ p_bbls->len;
					// Wrapped because it exceeds 90 characters
					// with 4 space tabs
					void *tmp = realloc(
						p_bbls->buf,
						sizeof(t_bbl) * p_bbls->len
					); ERR_WATCH;
					if (tmp == NULL) {
						free(p_bbls->buf);
						ERR_SET_G_ERROR("realloc fail");
						error();
					};

					p_bbls->buf = (t_bbl*)tmp;
					p_bbls->buf[p_bbls->len - 1].x   = j;
					p_bbls->buf[p_bbls->len - 1].y   = i;
					p_bbls->buf[p_bbls->len - 1].clr = rand() % 6 + 2;
				};
			} else if (tree[i][j] == '*')
				started = true;
		};
	};
};

t_bbl *bbls_find(t_bbls *p_bbls, uti p_x, uti p_y) {
	for (usize i = 0; i < p_bbls->len; ++ i) {
		if (p_bbls->buf[i].x == p_x && p_bbls->buf[i].y == p_y)
			return &p_bbls->buf[i];
	};

	return NULL;
};

// Snowflakes
void flakes_new(t_flakes *p_flakes, uti p_size_x, uti p_size_y) {
	ERR_WATCH_INIT;
	uti flake_cnt_x = p_size_x / (p_size_x / 2);
	p_flakes->len = flake_cnt_x * p_size_y;
	if (p_flakes->len < 1)
		p_flakes->len = 1;
	p_flakes->buf = (t_flake*)malloc(sizeof(t_flake) * p_flakes->len); ERR_WATCH;
	if (p_flakes->buf == NULL) {
		ERR_SET_G_ERROR("malloc fail");
		return;
	};

	uti x = 0;
	uti y = 0;
	for (usize i = 0; i < p_flakes->len; ++ i) {
		p_flakes->buf[i].y = -y;
		p_flakes->buf[i].x = rand() % p_size_x;

		++ x;
		if (x >= flake_cnt_x) {
			x = 0;
			++ y;
		};
	};
};

void flakes_resize(t_flakes *p_flakes, uti p_size_x, uti p_size_y) {
	ERR_WATCH_INIT;
	uti flake_cnt_x = p_size_x / (p_size_x / 2);
	p_flakes->len = flake_cnt_x * p_size_y;
	if (p_flakes->len < 1)
		p_flakes->len = 1;
	void *tmp = realloc(p_flakes->buf, sizeof(t_flake) * p_flakes->len); ERR_WATCH;
	if (tmp == NULL) {
		free(p_flakes->buf);
		ERR_SET_G_ERROR("realloc fail");
		return;
	}
	p_flakes->buf = (t_flake*)tmp;

	uti x = 0;
	uti y = 0;
	for (usize i = 0; i < p_flakes->len; ++ i) {
		p_flakes->buf[i].y = -y;
		p_flakes->buf[i].x = rand() % p_size_x;

		++ x;
		if (x >= flake_cnt_x) {
			x = 0;
			++ y;
		};
	};
};

void flakes_draw(t_flakes *p_flakes, t_scr *p_scr) {
	p_scr->clr = CLRID_WHITE;
	for (usize i = 0; i < flakes.len; ++ i) {
		if (tick % 100 == 0 && ++ flakes.buf[i].y >= p_scr->size_y) {
			flakes.buf[i].y = -1;
			flakes.buf[i].x = rand() % p_scr->size_x;
		} else
			scr_setcharat(p_scr, '*', flakes.buf[i].x, flakes.buf[i].y);
	};
};

// Main functions
void init(void) {
	initscr();
	noecho();
	start_color();
	use_default_colors();
	curs_set(0);
	timeout(0);
	raw();
	cbreak();

	init_pair(CLRID_RED,     COLOR_RED,     COLOR_B_DEFAULT);
	init_pair(CLRID_GREEN,   COLOR_GREEN,   COLOR_B_DEFAULT);
	init_pair(CLRID_YELLOW,  COLOR_YELLOW,  COLOR_B_DEFAULT);
	init_pair(CLRID_BLUE,    COLOR_BLUE,    COLOR_B_DEFAULT);
	init_pair(CLRID_MAGENTA, COLOR_MAGENTA, COLOR_B_DEFAULT);
	init_pair(CLRID_CYAN,    COLOR_CYAN,    COLOR_B_DEFAULT);
	init_pair(CLRID_WHITE,   COLOR_WHITE,   COLOR_B_DEFAULT);

	// Make rand() generate "truly" random numbers
	usize tmp = rand();
	srand(time(&tmp));

	scr_new(&scr, getmaxx(stdscr), getmaxy(stdscr));
	scr_clear(&scr);
	if (g_error.hpnd)
		error();

	tree_pos_x = scr.size_x / 2 - TREE_WIDTH / 2;
	tree_pos_y = scr.size_y / 2 - TREE_HEIGHT / 2;

	bbls_new(&bbls);
	flakes_new(&flakes, scr.size_x, scr.size_y);
};

void finish(void) {
	scr_free(&scr);
	if (g_error.hpnd)
		error();

	free(bbls.buf);
	free(flakes.buf);

	endwin();
};

void events(void) {
	u16 in = getch();
	switch (in) {
	case KEY_RESIZE:
		scr_resize(&scr, getmaxx(stdscr), getmaxy(stdscr));
		flakes_resize(&flakes, scr.size_x, scr.size_y);
		if (g_error.hpnd)
			error();

		tree_pos_x = scr.size_x / 2 - TREE_WIDTH / 2;
		tree_pos_y = scr.size_y / 2 - TREE_HEIGHT / 2;
		break;

	case 'q' & 31: // CTRL mask
		running = false;
	};
};

void render(void) {
	flakes_draw(&flakes, &scr);

	// Tree drawing
	for (usize i = 0; i < TREE_HEIGHT; ++ i) {
		if (i == TREE_IDX_STAR)
			scr.clr = CLRID_YELLOW;
		else if (i >= TREE_IDX_TEXT)
			scr.clr = CLRID_WHITE;
		else if (i >= TREE_IDX_STEM)
			scr.clr = CLRID_RED;
		else {
			for (usize j = 0; j < TREE_WIDTH; ++ j) {
				scr.clr = CLRID_GREEN;
				char ch = tree[i][j];

				t_bbl *bbl = bbls_find(&bbls, j, i);
				if (bbl != NULL) {
					if (tick % 700 == 0)
						bbl->clr = rand() % 6 + 2;
					ch = 'o';
					scr.clr = bbl->clr;
				};

				if (ch != ' ')
					scr_setcharat(&scr, ch, tree_pos_x + j, tree_pos_y + i);
			};
			continue;
		};

		for (usize j = 0; j < TREE_WIDTH; ++ j) {
			if (tree[i][j] != ' ')
				scr_setcharat(&scr, tree[i][j], tree_pos_x + j, tree_pos_y + i);
		};
	};

	scr_draw(&scr);
	if (g_error.hpnd)
		error();
};

void main_loop(void) {
	running = true;
	while (running) {
		events();
		render();

		usleep(600);
		 ++ tick;
	};
};

int main(int argc, const char *argv[]) {
	bool start = true;
	for (usize i = 1; i < argc; ++ i) {
		if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
			printf(
				"ltree version %i.%i.%i\n",
				VERSION_MAJOR,
				VERSION_MINOR,
				VERSION_PATCH
			);
			start = false;
		} else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			printf(
				"Usage: app [options]\n"
				"Options:\n"
				"    -h, --help     Show the usage\n"
				"    -v, --version  Show the current version\n"
			);
			start = false;
		} else {
			printf("Invalid parameter '%s' :(\nuse -h to see the usage\n", argv[i]);
			return 0;
		};
	};

	if (!start)
		return 0;

	init();
	main_loop();
	finish();

	return 0;
};
