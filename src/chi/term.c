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

vec term_get_size()
{
	struct winsize w = {};
	int z = ioctl(1, TIOCGWINSZ, &w);
	if (z < 0) {
		perror("window_get_size: ioctl() failed");
	}
	return v(w.ws_row, w.ws_col);
}

static struct termios termios_initial = {};

static void term_restore()
{
	__efail_if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios_initial));
	__efail_if(fprintf(stdout, "\x1b[?1004l"));
	__efail_if(fprintf(stdout, "\x1b[?1002l"));
	__efail_if(fprintf(stdout, "\x1b[?1000l"));
	__efail_if(fprintf(stdout, "\x1b[?47l"));
	__efail_if(fprintf(stdout, "\x1b[u"));
	__efail_if(fflush(stdout));
}


void term_init()
{
	int z;

	__efail_if(tcgetattr(STDIN_FILENO, &termios_initial));
	atexit(term_restore);

	__efail_if(fprintf(stdout, "\x1b[s"));
	__efail_if(fprintf(stdout, "\x1b[?47h"));
	__efail_if(fprintf(stdout, "\x1b[?1000h"));
	__efail_if(fprintf(stdout, "\x1b[?1002h"));
	__efail_if(fprintf(stdout, "\x1b[?1004h"));
	__efail_if(fflush(stdout));

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
	termios_raw.c_cc[VTIME] = 100;			// 100 * 100 ms timeout
	termios_raw.c_cflag |= CS8;                        // 8 bits chars
	__efail_if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios_raw));
}

void term_render(struct buffer buffer, struct term_frame *frame)
{
	struct slice slice = buffer_to_slice(buffer);

	// How to use slice for copying immutable const char* into a mutable slice ?
	slice = slice_copy_string(slice, term_clear);
	slice = slice_copy_string(slice, term_cursor_hide);
	slice = slice_copy_string(slice, term_cursor_show);
}
