#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <chi/base.h>
#include <chi/geometry.h>
#include <chi/memory.h>
#include <chi/error.h>
#include <chi/io.h>


/***********
 * LOGGING *
 ***********/


#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>

// The fd value where logs are emitted. Goes to /dev/null by default.
static const int LOG_FILENO = 77;
// Default size of the log buffer for string assemlbing
static const int LOG_BUFFER_SIZE = 256;

// Run this at startup to be sure that fd LOG_FILENO is reserved.
int init_log()
{
	int fd = open("/dev/null", O_WRONLY);
	if (fd < 0) {
		return -1;
	}

	int r = dup2(fd, LOG_FILENO);
	if (r < 0) {
		close(fd);
		return -1;
	}

	return 0;
}

int log_formatted_message(const char *format, ...)
{
	int s = LOG_BUFFER_SIZE;
	char buffer[s];

	va_list(args);
	va_start(args, format);
	int r = vsnprintf(buffer, s, format, args);
	va_end(args);
	if (s <= r) {
		r = s - 1;
	}

	__assert1(r < s);
	buffer[r] = '\n';
	return write(LOG_FILENO, buffer, r + 1);
}

#define logm(format, ...)  log_formatted_message(__LOC__ " %s [ " format, __func__, ##__VA_ARGS__)
#define logs(s)  logm("%s", s)



/************
 * TERMINAL *
 ************/

#include <errno.h>
#include <termios.h>

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
		// TODO: softer error checking
		perror("window_get_size: ioctl() failed");
		exit(1);
	}
	return v(w.ws_row, w.ws_col);
}

static struct termios termios_initial = {};

void term_restore()
{
	__efail_if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios_initial));
	__efail_if(fprintf(stdout, "\x1b[?1004l"));
	__efail_if(fprintf(stdout, "\x1b[?1002l"));
	__efail_if(fprintf(stdout, "\x1b[?1000l"));
	__efail_if(fprintf(stdout, "\x1b[?47l"));
	__efail_if(fprintf(stdout, "\x1b[u"));
	__efail_if(fflush(stdout));
}


void term_raw()
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

struct term_frame {
	// TODO
};

void term_render(struct buffer buffer, struct term_frame *frame)
{
	struct slice slice = buffer_to_slice(buffer);

	// How to use slice for copying immutable const char* into a mutable slice ?
	slice = slice_copy_string(slice, term_clear);
	slice = slice_copy_string(slice, term_cursor_hide);
	slice = slice_copy_string(slice, term_cursor_show);
}




/********
 * TEST *
 ********/

int is_space(u8 c) {
	return c == ' ' || c == '\t';
}

int is_not_space(u8 c) {
	return !is_space(c);
}

struct keyval {
	struct slice key;
	struct slice val;
};

struct keyval keyval_from_line(struct slice s)
{
	struct keyval kv;

	s = slice_split(&s, '#');

	slice_while(&s, is_space);
	kv.key = slice_while(&s, is_not_space);

	slice_while(&s, is_space);
	kv.val = slice_while(&s, is_not_space);

	return kv;
}

struct configuration {
	int argument_a;
	int argument_b;
};

void blo() { backtrace_print(); }
void bli() { blo(); }
void bla() { bli(); }

int main_kv(int argc, char** args)
{
	struct mapped_file config;
	mapped_file_load(&config, "./config.txt");

	while (slice_len(config.data)) {
		struct slice line = slice_take_line(&config.data);
		if (slice_empty(line)) {
			continue;
		}

		struct keyval kv = keyval_from_line(line);
		if (slice_empty(kv.key)) {
			continue;
		}

		char* k = slice_to_string(kv.key);
		char* v = slice_to_string(kv.val);
		if (slice_empty(kv.val)) {
			printf("W: no val for key '%s'\n", k);
		} else {
			printf("key:%s val:%s\n", k, v);
		}

		if (k) free(k);
		if (v) free(v);
	}
}

/********
 * MAIN *
 ********/


int main(int argc, char **args)
{
	puts(*args);

	read_binary(*args);

	logm("enter");

	main_kv(argc, args);

	puts("");
	puts("print backtrace:");
	bla();

	if (1) {
		return 0;
	}

	//term_raw();

	char buffer[64] = {};
	struct slice ss = s(buffer, buffer + 64);
	//size_t l = mylen(ss);

	for (int i = 0; i < argc; i++) {
		puts(args[i]);
		puts("\n");
		logm("this is a log:%s", args[i]);
		logs(args[i]);
	}

	logm("exit");
}
