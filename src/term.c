#include <chi.h>

#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>

#define TERM_NEWLINE		"\r\n"
#define TERM_ESC		"\x1b"

#define DEBUG			0

#define NUMCOLOR 256
static const char* fg_color_control_string[NUMCOLOR];
static const char* bg_color_control_string[NUMCOLOR];

// TODO: put this into a config file ?
static const int default_color_fg = 1;
static const int default_color_bg = 0;
static const char default_text = '?';

static void color_init() {
	size_t stringlen = strlen(";48;5;" __stringize2(NUMCOLOR) "m") + /* null byte */ 1;
	char* buffer = malloc(2 * NUMCOLOR * stringlen);
	// if needed: free(fg_color_control_string[0]);
	for (int i = 0; i < NUMCOLOR; i++) {
		sprintf(buffer, "38;5;%d", i);
		fg_color_control_string[i] = buffer;
		buffer += stringlen;
		sprintf(buffer, ";48;5;%dm", i);
		bg_color_control_string[i] = buffer;
		buffer += stringlen;
	}
}

static void color_start(struct buffer* buffer, int fg, int bg)
{
	assert_range(0, fg, NUMCOLOR - 1);
	assert_range(0, bg, NUMCOLOR - 1);
	buffer_append_cstring(buffer, TERM_ESC "[");
	buffer_append_cstring(buffer, fg_color_control_string[fg]);
	buffer_append_cstring(buffer, bg_color_control_string[bg]);
}

static void color_stop(struct buffer* buffer)
{
	buffer_append_cstring(buffer, TERM_ESC "[0m");
}

static const char term_setup_sequence[] =
	TERM_ESC "[s"             // cursor save
	TERM_ESC "[?47h"          // switch offscreen
	TERM_ESC "[?1000h"        // mouse event on
	TERM_ESC "[?1002h"        // mouse tracking on
	TERM_ESC "[?1004h"        // switch focus event on
	;

static const char term_restore_sequence[] =
	TERM_ESC "[?1004l"        // switch focus event off
	TERM_ESC "[?1002l"        // mouse tracking off
	TERM_ESC "[?1000l"        // mouse event off
	TERM_ESC "[?47l"          // switch back to main screen
	TERM_ESC "[u"             // cursor restore
	;

static struct termios termios_initial = {};
static int term_init_in_fd;
static int term_init_out_fd;

static void term_restore()
{
	assert_success(tcsetattr(term_init_in_fd, TCSAFLUSH, &termios_initial));
	assert_success(write(term_init_out_fd, term_restore_sequence, strlen(term_restore_sequence)));
}

void term_init(int term_in_fd, int term_out_fd)
{
	color_init();

if (CONFIG.debug_noterm) return;

	term_init_in_fd = term_in_fd;
	term_init_out_fd = term_out_fd;

	assert_success(tcgetattr(term_in_fd, &termios_initial));

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

	assert_success(write(term_out_fd, term_setup_sequence, strlen(term_setup_sequence)));
	assert_success(tcsetattr(term_in_fd, TCSAFLUSH, &termios_raw));
	atexit(term_restore);
}

#define iter_line_min(iter_p)	((iter_p)->window).y0
#define iter_line_max(iter_p)	((iter_p)->window).y1
#define iter_line_len(iter_p)	rec_w((iter_p)->window)
#define iter_height(iter_p)	rec_h((iter_p)->window)
#define iter_offset(iter_p)	((iter_p)->stride * (iter_p)->line + (iter_p)->window.x0)

struct framebuffer_iter framebuffer_iter_make(struct framebuffer *framebuffer, rec rec)
{
	clamp_rec(&rec, framebuffer->window);
	return (struct framebuffer_iter) {
		.text = framebuffer->text,
		.fg = framebuffer->fg_colors,
		.bg = framebuffer->bg_colors,
		.stride = framebuffer->window.x,
		.window = rec,
		// Start just before the first line. Empty iterator
		// will correctly not move forward.
		.line = rec.y0 - 1,
	};
}

void framebuffer_iter_reset_forward(struct framebuffer_iter *iter)
{
	iter->line = iter_line_min(iter) - 1;
}

void framebuffer_iter_reset_backward(struct framebuffer_iter *iter)
{
	iter->line = iter_line_max(iter) + 1;
}

void framebuffer_iter_goto(struct framebuffer_iter *iter, int n)
{
	iter->line = clamp(n, iter_line_min(iter), iter_line_max(iter));
}

void framebuffer_iter_move(struct framebuffer_iter *iter, int n)
{
	framebuffer_iter_goto(iter, iter->line + n);
}

int framebuffer_iter_next(struct framebuffer_iter *iter)
{
	int min = iter_line_min(iter);
	int max = iter_line_max(iter);
	assert_range(min - 1, iter->line, max + 1);
	if (max <= iter->line) {
		return 0;
	}
	iter->line++;
	return 1;
}

int framebuffer_iter_prev(struct framebuffer_iter *iter)
{
	int min = iter_line_min(iter);
	int max = iter_line_max(iter);
	assert_range(min - 1, iter->line, max + 1);
	if (iter->line <= min) {
		return 0;
	}
	iter->line--;
	return 1;
}

size_t framebuffer_push_text(struct framebuffer_iter *iter, char *text, size_t size)
{
	assert_range(iter_line_min(iter), iter->line, iter_line_max(iter));
	size = min(size, (size_t) iter_line_len(iter));
	memcpy(iter->text + iter_offset(iter), text, size);
	return size;
}

size_t framebuffer_push_fg(struct framebuffer_iter *iter, int fg, size_t size)
{
	assert_range(iter_line_min(iter), iter->line, iter_line_max(iter));
	size = min(size, (size_t) iter_line_len(iter));
	memset_i32(iter->fg + iter_offset(iter), fg, size * sizeof(int));
	return size;
}

size_t framebuffer_push_bg(struct framebuffer_iter *iter, int bg, size_t size)
{
	assert_range(iter_line_min(iter), iter->line, iter_line_max(iter));
	size = min(size, (size_t) iter_line_len(iter));
	memset_i32(iter->bg + iter_offset(iter), bg, size * sizeof(int));
	return size;
}

void framebuffer_init(struct framebuffer *framebuffer, vec term_size)
{
	assert(framebuffer);

	framebuffer->window = term_size;
	size_t grid_size = term_size.x * term_size.y;
	framebuffer->buffer_len = grid_size;
	framebuffer->text = realloc(framebuffer->text, grid_size);
	framebuffer->fg_colors = realloc(framebuffer->fg_colors, sizeof(int) * grid_size);
	framebuffer->bg_colors = realloc(framebuffer->bg_colors, sizeof(int) * grid_size);

	buffer_ensure_size(&framebuffer->output_buffer, 0x10000);
	memset(framebuffer->text, default_text, grid_size);
	memset_i32(framebuffer->fg_colors, default_color_fg, sizeof(int) * grid_size);
	memset_i32(framebuffer->bg_colors, default_color_bg, sizeof(int) * grid_size);

	assert(framebuffer->text);
	assert(framebuffer->fg_colors);
	assert(framebuffer->bg_colors);
	assert(framebuffer->output_buffer.memory);
}

void framebuffer_draw_to_term(int term_out_fd, struct framebuffer *framebuffer, vec cursor)
{
	struct buffer buffer = framebuffer->output_buffer;
	buffer.cursor = 0;

	// How to use slice for copying immutable const char* into a mutable slice ?
	buffer_append_cstring(&buffer, TERM_ESC "" "c");		// clear term screen
	buffer_append_cstring(&buffer, TERM_ESC "[?25l");		// hide cursor to avoid cursor blinking
	buffer_append_cstring(&buffer, TERM_ESC "[H");			// go home, i.e top left

	vec window = framebuffer->window;
	char *text = framebuffer->text;
	char *text_end = text + window.x * window.y;
	int *fg = framebuffer->fg_colors;
	int *bg = framebuffer->bg_colors;
	int x = 0;
debugf("text,text_end: %p,%p\n", text, text_end);
ifdebug for (int i = 0; i < 40; i++) { //window.x * window.y; i++) {
	printf("%d:[%d,%d] ", i, *(framebuffer->fg_colors + i), *(framebuffer->bg_colors + i));
}
debugf("\n");
	while (text < text_end) {
		char *section_start = text;
		int current_fg = *fg;
		int current_bg = *bg;
debugf("section color offset +x:%ld fg:%d bg:%d\n", (ssize_t)(section_start - text), current_fg, current_bg);
		color_start(&buffer, current_fg, current_bg);
		while (x < window.x && *fg == current_fg && *bg == current_bg) {
			x++;
			text++;
			fg++;
			bg++;
		}
		buffer_append(&buffer, section_start, text - section_start);
		color_stop(&buffer);
		if (x == window.x) {
			buffer_append_cstring(&buffer, TERM_NEWLINE);
			x = 0;
		}
	}

	// put cursor: terminal cursor positions start at (1,1) instead of (0,0).
	buffer_appendf(&buffer, TERM_ESC "[%d;%dH", cursor.x + 1, cursor.y + 1);
	buffer_append_cstring(&buffer, TERM_ESC "[?25h");		// show cursor

debugf("buffer cursor:%lu\n", buffer.cursor);
	assert_success(write(term_out_fd, buffer.memory, buffer.cursor));
	// TODO: instead return error_because(errno);
}

void framebuffer_clear(struct framebuffer *framebuffer, rec rec)
{
	struct framebuffer_iter iter = framebuffer_iter_make(framebuffer, rec);
	while (framebuffer_iter_next(&iter)) {
debugf("iterator current:%d max:%d\n", iter.line, iter_line_max(&iter));
		memset(iter.text, default_text, iter_line_len(&iter));
		framebuffer_push_fg(&iter, default_color_fg, iter_line_len(&iter));
		framebuffer_push_bg(&iter, default_color_bg, iter_line_len(&iter));
	}
}

void fill_color_rec(int *color_array, vec window, int color, rec rec)
{
	char buf[256];
	puts(buf);
	clamp_rec(&rec, window);
	puts(buf);
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
void framebuffer_print(char *buffer, size_t size, struct framebuffer *framebuffer)
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

static struct input term_get_mouse_input(int term_in_fd);

// This function is not re-entrant. To make it re-entrant, stdin needs to be parametrized as a fd value since
// term_get_input needs to own and see all the input byte stream on standard input anyway.
// Making it re-entrant like so could help with testing or multi client / multi terminal setups.
// Note that for testing only use cases, the stdin fd can be redirected with dup() anyway.
struct input term_get_input(int term_in_fd)
{
	// Consume any pending input first
	if (pending_input_cursor < pending_input_end) {
		return input_for_key(*pending_input_cursor++);
	}

	memset(input_buffer, 0, sizeof(input_buffer));
	ssize_t n = 0;
	// If timeout is set low enough, this turns into a polling loop.
	while (!(n = read(term_in_fd, input_buffer, sizeof(input_buffer)))) {
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
			return term_get_mouse_input(term_in_fd);
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

static struct input term_get_mouse_input(int term_in_fd)
{
	memset(input_buffer, 0, sizeof(input_buffer));
	const ssize_t n = read(term_in_fd, input_buffer, sizeof(input_buffer));
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
