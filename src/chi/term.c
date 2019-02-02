#include <chi/term.h>

#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>

#include <chi/config.h>
#include <chi/base.h>

static const char term_seq_finish[]                       = "\x1b[0m";
static const char term_newline[]                          = "\r\n";

static struct termios termios_initial = {};

static const char term_setup_sequence[] =
	"\x1b[s"             // cursor save
	"\x1b[?47h"          // switch offscreen
	"\x1b[?1000h"        // mouse event on
	"\x1b[?1002h"        // mouse tracking on
	"\x1b[?1004h"        // switch focus event on
	;

static const char term_restore_sequence[] =
	"\x1b[?1004l"        // switch focus event off
	"\x1b[?1002l"        // mouse tracking off
	"\x1b[?1000l"        // mouse event off
	"\x1b[?47l"          // switch back to main screen
	"\x1b[u"             // cursor restore
	;

static void term_restore();
void term_init()
{

if (CONFIG.debug_noterm) return;

	assert_success(tcgetattr(STDIN_FILENO, &termios_initial));

	struct termios termios_raw = termios_initial;
	// Input modes
	termios_raw.c_iflag &= ~INPCK;                  // no parity check
	termios_raw.c_iflag &= ~PARMRK;			// no parity check
	termios_raw.c_iflag &= ~ISTRIP;                 // no strip character
	termios_raw.c_iflag &= ~IGNBRK;			// do not ignore break conditions
	termios_raw.c_iflag &= ~BRKINT;                 // no break
	termios_raw.c_iflag &= ~IGNCR; 			// keep CR on RET key
	termios_raw.c_iflag &= ~ICRNL;                  // no CR to NL
	termios_raw.c_iflag &= ~INLCR; 			// no NL to CR conversion
	termios_raw.c_iflag &= ~IXON;                   // no CR to NL
	termios_raw.c_iflag &= ~IXOFF; 			// no start/stop control chars on input
	// Output modes
	termios_raw.c_oflag &= ~OPOST;			// no post processing
	// Local modes
	termios_raw.c_lflag &= ~ECHO;			// no echo
	termios_raw.c_lflag &= ~ECHONL;			// no NL echo
	termios_raw.c_lflag &= ~ICANON;			// no canonical mode, read input as soon as available
	termios_raw.c_lflag &= ~IEXTEN;			// no extension
	termios_raw.c_lflag &= ~ISIG;			// turn off sigint, sigquit, sigsusp
	termios_raw.c_cc[VMIN] = 0;			// return each byte, or nothing when timeout
	termios_raw.c_cc[VTIME] = 100;			// VTIME x 100 ms timeout
	termios_raw.c_cflag |= CS8;                     // 8 bits chars

	assert_success(write(STDOUT_FILENO, term_setup_sequence, strlen(term_setup_sequence)));
	assert_success(tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios_raw));
	atexit(term_restore);
}

static void term_restore()
{
	assert_success(tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios_initial));
	assert_success(write(STDOUT_FILENO, term_restore_sequence, strlen(term_restore_sequence)));
}

static const char* term_color_string(int fg, int bg)
{
	// TODO: decide on color encoding
	return "";
}

void framebuffer_init(struct framebuffer* framebuffer, vec term_size)
{
	assert(framebuffer);

	framebuffer->window = term_size;
	size_t grid_size = term_size.x * term_size.y;
	framebuffer->buffer_len = grid_size;
	framebuffer->text = realloc(framebuffer->text, grid_size);
	framebuffer->fg_colors = realloc(framebuffer->fg_colors, sizeof(int) * grid_size);
	framebuffer->bg_colors = realloc(framebuffer->bg_colors, sizeof(int) * grid_size);

	buffer_ensure_size(&framebuffer->output_buffer, 0x10000);

	assert(framebuffer->text);
	assert(framebuffer->fg_colors);
	assert(framebuffer->bg_colors);
	assert(framebuffer->output_buffer.memory);
}

void framebuffer_draw_to_term(struct framebuffer *framebuffer, vec cursor)
{
	struct buffer buffer = framebuffer->output_buffer;
	buffer.cursor = 0;

	// How to use slice for copying immutable const char* into a mutable slice ?
	buffer_append_cstring(&buffer, "\x1b" "c");		// clear term screen
	buffer_append_cstring(&buffer, "\x1b[?25l");		// hide cursor to avoid cursor blinking
	buffer_append_cstring(&buffer, "\x1b[H");		// go home, i.e top left

	vec window = framebuffer->window;
	c8* text = framebuffer->text;
	c8* text_end = text + window.x * window.y;
	int *fg = framebuffer->fg_colors;
	int *bg = framebuffer->bg_colors;
	int x = 0;
	while (text < text_end) {
		c8* section_start = text;
		int current_fg = *fg;
		int current_bg = *bg;
		buffer_append_cstring(&buffer, term_color_string(current_fg, current_bg));
		// Start sequence for colored string
		while (x < window.x && *fg == current_fg && *bg == current_bg) {
			x++;
			text++;
			fg++;
			bg++;
		}
		buffer_append(&buffer, section_start, text - section_start);
		buffer_append_cstring(&buffer, "\027[0m"); // Finish sequence for colored string
		if (x == window.x) {
			buffer_append_cstring(&buffer, "\r\n");
			x = 0;
		}
	}

	// put cursor: terminal cursor positions start at (1,1) instead of (0,0).
	buffer_appendf(&buffer, "\027[%d;%dH", cursor.x + 1, cursor.y + 1);
	buffer_append_cstring(&buffer, "\x1b[?25h");		// show cursor

	assert_success(write(STDOUT_FILENO, buffer.memory, buffer.cursor));
	// TODO: instead return error_because(errno);
}

static int clamp(int v, int lo, int hi)
{
	if (v < lo) return lo;
	if (hi < v) return hi;
	return v;
}

static void clamp_rec(rec *rec, vec window)
{
	rec->x0 = clamp(rec->x0, 0, window.x);
	rec->y0 = clamp(rec->y0, 0, window.y);
	rec->x1 = clamp(rec->x1, rec->x0, window.x);
	rec->y1 = clamp(rec->y1, rec->y0, window.y);
}

void framebuffer_clear(struct framebuffer *framebuffer, rec rec)
{
	// TODO: put this into a config file ?
	static const int default_color_fg = 1;
	static const int default_color_bg = 0;
	static const c8 default_text = ' ';

	struct framebuffer_iter iter = framebuffer_iter_make(framebuffer, rec);
	while (framebuffer_iter_next(&iter)) {
		memset(iter.text, default_text, iter.line_length);
		framebuffer_push_fg(&iter, default_color_fg, iter.line_length);
		framebuffer_push_bg(&iter, default_color_bg, iter.line_length);
	}
}

void fill_color_rec(int* color_array, vec window, int color, rec rec)
{
	clamp_rec(&rec, window);
	for (int y = rec.y0; y < rec.y1; y++) {
		for (int x = rec.x0; x < rec.x1; x++) {
			*(color_array + window.x * y + x) = color;
		}
	}
}

void framebuffer_put_color_fg(struct framebuffer *framebuffer, int fg, rec rec)
{
	fill_color_rec(framebuffer->fg_colors, framebuffer->window, fg, rec);
}

void framebuffer_put_color_bg(struct framebuffer *framebuffer, int bg, rec rec)
{
	fill_color_rec(framebuffer->bg_colors, framebuffer->window, bg, rec);
}

struct framebuffer_iter framebuffer_iter_make(struct framebuffer *framebuffer, rec rec)
{
	vec window = framebuffer->window;
	clamp_rec(&rec, window);
	return (struct framebuffer_iter) {
		.text = framebuffer->text,
		.fg = framebuffer->fg_colors,
		.bg = framebuffer->bg_colors,
		.stride = window.x,
		.line_length = rec_w(rec),
		.line_max = rec_h(rec),
		// Start just before the first line. Empty iterator with
		// line_max = 0 will correctly not moved forward.
		.line_current = -1,
	};
}

void framebuffer_iter_offset(struct framebuffer_iter *iter, size_t offset)
{
	offset = min(offset, iter->line_length);
	iter->line_length -= offset;
	iter->text += offset;
	iter->fg += offset;
	iter->bg += offset;
}
void framebuffer_iter_reset_first(struct framebuffer_iter *iter)
{
	iter->line_current = -1;
}

void framebuffer_iter_reset_last(struct framebuffer_iter *iter)
{
	iter->line_current = iter->line_max;
}

int framebuffer_iter_next(struct framebuffer_iter* iter)
{
	assert(-1 <= iter->line_current);
	assert(iter->line_current <= iter->line_max);
	int done = (iter->line_current == iter->line_max);
	if (!done) {
		iter->text += iter->stride;
		iter->fg += iter->stride * sizeof(int);
		iter->bg += iter->stride * sizeof(int);
		iter->line_current++;
	}
	return done;
}

int framebuffer_iter_prev(struct framebuffer_iter* iter)
{
	assert(-1 <= iter->line_current);
	assert(iter->line_current <= iter->line_max);
	int done = (iter->line_current == -1);
	if (!done) {
		iter->text -= iter->stride;
		iter->fg -= iter->stride * sizeof(int);
		iter->bg -= iter->stride * sizeof(int);
		iter->line_current--;
	}
	return done;
}

size_t framebuffer_push_text(struct framebuffer_iter *iter, c8 *text, size_t size)
{
	size = min(size, iter->line_length);
	memcpy(iter->text, text, size);
	return size;
}

size_t framebuffer_push_fg(struct framebuffer_iter *iter, int fg, size_t size)
{
	size = min(size, iter->line_length);
	// TODO: I need a 4bytes memset !
	for (size_t x = 0; x < iter->line_length; x++) {
		*(iter->fg + x) = fg;
	}
	return size;
}

size_t framebuffer_push_bg(struct framebuffer_iter *iter, int bg, size_t size)
{
	size = min(size, iter->line_length);
	for (size_t x = 0; x < iter->line_length; x++) {
		*(iter->bg + x) = bg;
	}
	return size;
}

vec term_get_size()
{
	struct winsize w = {};
	if (ioctl(1, TIOCGWINSZ, &w) < 0) {
		perror("window_get_size: ioctl() failed");
	}
	return v(w.ws_row, w.ws_col);
}

static inline struct input input_for_error(int err) {
	return (struct input) {
		.code = INPUT_ERROR_CODE,
		.errno_value = err,
	};
}

static inline struct input input_for_code(enum input_code code) {
	return (struct input) {
		.code = code,
	};
}

static inline struct input input_for_key(char code) {
	return (struct input) {
		.code = code,
	};
}

// debugging functions
void framebuffer_print(char* buffer, size_t size, struct framebuffer* framebuffer)
{
  snprintf(buffer, size, "framebuffer { .x=%d .y=%d .buffer_len=%lu .text=%p .fg_colors=%p .bg_colors=%p .buffer.memory=%p .buffer.cursor=%lu .buffer.size=%lu }",
  framebuffer->window.x,
  framebuffer->window.y,
  framebuffer->buffer_len,
  framebuffer->text,
  framebuffer->fg_colors,
  framebuffer->bg_colors,
  framebuffer->output_buffer.memory,
  framebuffer->output_buffer.cursor,
  framebuffer->output_buffer.size);
}


static char input_buffer[3] = {};
static char *pending_input_cursor = NULL;
static char *pending_input_end = NULL;

static struct input term_get_mouse_input();

// This function is not re-entrant. To make it re-entrant, stdin needs to be parametrized as a fd value since
// term_get_input needs to own and see all the input byte stream on standard input anyway.
// Making it re-entrant like so could help with testing or multi client / multi terminal setups.
// Note that for testing only use cases, the stdin fd can be redirected with dup() anyway.
struct input term_get_input()
{
	// Consume any pending input first
	if (pending_input_cursor < pending_input_end) {
		return input_for_key(*pending_input_cursor++);
	}

	memset(input_buffer, 0, sizeof(input_buffer));
	ssize_t n = 0;
	// If timeout is set low enough, this turns into a polling loop.
	while (!(n = read(STDIN_FILENO, input_buffer, sizeof(input_buffer)))) {
//printf("%s: %i,%i,%i\n\r", "get_input: read", input_buffer[0], input_buffer[1], input_buffer[2]);
	}

	// Sanity check
	if (n > 3) {
		return input_for_error(EOVERFLOW);
	}

	// Terminal resize events send SIGWINCH signals which interrupt read()
	if (n < 0 && errno == EINTR) {
		return input_for_code(INPUT_RESIZE_CODE);
	}

	// Other error conditions
	if (n < 0) {
		return input_for_error(errno);
	}

	// One normal key
	if (n == 1) {
		return input_for_key(input_buffer[0]);
	}

	// Escape sequences of the form "\x1b[ + key (+ opt data for mouse clicks).
	// Unfortunately typing CTRL + [ and a key in the known cases below will trigger these codepaths.
	if (n == 3 && input_buffer[1] == '[') {
		switch (input_buffer[2]) {
		case 'Z':
			return input_for_code(INPUT_KEY_ESCAPE_Z);
		case 'A':
			return input_for_code(INPUT_KEY_ARROW_UP);
		case 'B':
			return input_for_code(INPUT_KEY_ARROW_DOWN);
		case 'C':
			return input_for_code(INPUT_KEY_ARROW_RIGHT);
		case 'D':
			return input_for_code(INPUT_KEY_ARROW_LEFT);
		case 'M':
			return term_get_mouse_input();
		}
	}

	// Other inputs can happen when typing fast enough ctrl + [ and another key just after.
	// In that case, return the first key and queue the remaining input
	pending_input_cursor = input_buffer + 1;
	pending_input_end =  input_buffer + n;

	return input_for_key(input_buffer[0]);
}

static inline int mouse_coord_fixup(int coord) {
	coord -= 33;
	if (coord < 0) {
		coord += 255;
	}
	return coord;
}

static struct input term_get_mouse_input()
{
	memset(input_buffer, 0, sizeof(input_buffer));
	const ssize_t n = read(STDIN_FILENO, input_buffer, sizeof(input_buffer));
	if (n > 3) {
		return input_for_error(EOVERFLOW);
	}
	if (n < 0 && errno == EINTR) {
		return input_for_code(INPUT_RESIZE_CODE);
	}
	if (n < 0) {
		return input_for_error(errno);
	}

	static enum input_code mouse_click_types[] = {
		INPUT_MOUSE_LEFT,
		INPUT_MOUSE_MIDDLE,
		INPUT_MOUSE_RIGHT,
		INPUT_MOUSE_RELEASE,
	};

	return (struct input) {
		.code = mouse_click_types[input_buffer[0] & 0x3],
		.mouse_click.x = mouse_coord_fixup(input_buffer[1]),
		.mouse_click.y = mouse_coord_fixup(input_buffer[2]),
	};

}
