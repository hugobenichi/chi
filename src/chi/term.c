#include <chi/term.h>

#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>

#include <chi/base.h>

static const char term_seq_finish[]                       = "\x1b[0m";
static const char term_clear[]                            = "\x1b" "c";
static const char term_newline[]                          = "\r\n";
static const char term_cursor_hide[]                      = "\x1b[?25l";
static const char term_cursor_show[]                      = "\x1b[?25h";

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
	__efail_if(tcgetattr(STDIN_FILENO, &termios_initial));

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
	termios_raw.c_cc[VTIME] = 1;			// 1* 100 ms timeout
	termios_raw.c_cflag |= CS8;                     // 8 bits chars

	__efail_if(write(STDOUT_FILENO, term_setup_sequence, strlen(term_setup_sequence)) < 0);
	__efail_if(fsync(STDOUT_FILENO));
	__efail_if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios_raw));
	atexit(term_restore);
}

static void term_restore()
{
	__efail_if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios_initial));
	__efail_if(write(STDOUT_FILENO, term_restore_sequence, strlen(term_restore_sequence)) < 0);
	__efail_if(fsync(STDOUT_FILENO));
}

void term_framebuffer_init(struct term_framebuffer* framebuffer, vec term_size)
{
}

// TODO: initialize in term_init
static struct buffer renderbuffer;

void term_draw(struct term_framebuffer *framebuffer)
{
	struct slice slice = buffer_to_slice(renderbuffer);

	// How to use slice for copying immutable const char* into a mutable slice ?
	slice = slice_copy_string(slice, term_clear);
	slice = slice_copy_string(slice, term_cursor_hide);
	slice = slice_copy_string(slice, term_cursor_show);
}

vec term_get_size()
{
	struct winsize w = {};
	if (ioctl(1, TIOCGWINSZ, &w) < 0) {
		perror("window_get_size: ioctl() failed");
	}
	return v(w.ws_row, w.ws_col);
}

static inline struct input input_for_type(enum input_type type) {
	return (struct input) {
		.type = type,
	};
}

static inline struct input input_for_key(enum input_key_code code) {
	return (struct input) {
		.type = INPUT_KEY,
		.key = code,
	};
}

static char pending_input_buffer[3] = {};
static int pending_input_cursor = 0;
static int pending_input_lastread = 0;

struct input term_get_input()
{
	// Consume any pending input first
	if (pending_input_cursor > 0) {
		char c = pending_input_buffer[pending_input_cursor];
		pending_input_cursor++;
		if (pending_input_cursor == pending_input_lastread) {
			pending_input_cursor = 0;
		}
		return input_for_key(c);
	}

	const ssize_t n = read(STDIN_FILENO, pending_input_buffer, sizeof(pending_input_buffer));

	// Terminal resize events send SIGWINCH signals which interrupt read()
	if (n < 0 && errno == EINTR) {
		return input_for_type(INPUT_RESIZE);
	}

	// Other error conditions
	if (n < 0) {
		return input_for_type(INPUT_ERROR);
	}

	// One normal key
	if (n == 1) {
		return input_for_key(pending_input_buffer[0]);
	}

	// Escape sequences
	if (n == 3 && pending_input_buffer[1] == '[') {
		switch (pending_input_buffer[2]) {
		case 'Z':
			return input_for_key(INPUT_KEY_ESCAPE_Z);
		case 'A':
			return input_for_key(INPUT_KEY_ARROW_UP);
		case 'B':
			return input_for_key(INPUT_KEY_ARROW_DOWN);
		case 'C':
			return input_for_key(INPUT_KEY_ARROW_RIGHT);
		case 'D':
			return input_for_key(INPUT_KEY_ARROW_LEFT);
		case 'M':
			// TODO: mouse event !
			return input_for_type(INPUT_NONE);
/*
              Unix.read Unix.stdin buffer 0 input_buffer_len |> ignore ;
              (* x10 mouse click mode. *)
              (* TODO: add support for other modes: xterm-262, ... *)
              let x10_position_reader c =
                let c' = (Char.code c) - 33 in
                if c' < 0 then c' + 255 else c'
              in
              let cx = Bytes.get buffer 1 |> x10_position_reader in
              let cy = Bytes.get buffer 2 |> x10_position_reader in
              Bytes.get buffer 0
                |> Char.code
                |> (land) 3 (* Ignore modifier keys *)
                |> (function
                  (* TODO: distinguish between left/middle/right buttons *)
                  | 0
                  | 1
                  | 2   ->  Click (mk_v2 cx cy)
                  | 3   ->  ClickRelease (mk_v2 cx cy)
                  | cb  ->  fail (Printf.sprintf "unexpected mouse event %d,%d,%d" cb cx cy))
*/
		}
	}

	// Other inputs can happen when typing fast enough ctrl + [ and another key just after.
	// In that case, return the first key and queue the remaining input

	pending_input_cursor = 1;
	pending_input_lastread = n;

	return input_for_key(pending_input_buffer[0]);
}
