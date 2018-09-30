#include <stdio.h>
#include <assert.h>

#include <math.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <chi/geometry.h>

/*********
 * UTILS *
 *********/


#include <stdint.h>
typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;


#define Kilo(x) (1024L * (x))
#define Mega(x) (1024L * Kilo(x))
#define Giga(x) (1024L * Mega(x))

#define __stringize1(x) #x
#define __stringize2(x) __stringize1(x)
#define __LOC__ __FILE__ ":" __stringize2(__LINE__)

// Careful: the result expr gets evaluated twice
#define __max(x,y) ((x) > (y) ? : (x) : (y))
#define __min(x,y) ((x) < (y) ? : (x) : (y))

// Print formatted error msg and exit.
void fatal(const char * format, ...)
{
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        fprintf(stderr, "\n");
        exit(1);
}

// Check if condition is false or true respectively, otherwise print formatted message and exit.
#define __fail_if2(condition, format, ...) if (condition) fatal(__LOC__ " %s [ " format, __func__, ##__VA_ARGS__)
#define __assert2(condition, format, ...) if (!(condition)) fatal(__LOC__ " %s [ " format, __func__, ##__VA_ARGS__)
// Same, but print the failed condition instead of a formatted message.
#define __assert1(condition) __assert2(condition, "assert failed: \"" __stringize2(condition) "\"")
#define __fail_if1(condition) __fail_if2(condition, "fatal condition: \"" __stringize2(condition) "\"")


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

// CLEANUP: inline term constants !
// TODO: make these consistent with other termraw functions in github.com/hugobenichi/czl

//static const char term_seq_finish[]                       = "\x1b[0m";
//static const char term_clear[]                            = "\x1bc";
//static const char term_newline[]                          = "\r\n";
//static const char term_cursor_hide[]                      = "\x1b[?25l";
//static const char term_cursor_show[]                      = "\x1b[?25h";
static const char term_cursor_save[]                      = "\x1b[s";
static const char term_cursor_restore[]                   = "\x1b[u";
static const char term_switch_offscreen[]                 = "\x1b[?47h";
static const char term_switch_mainscreen[]                = "\x1b[?47l";
static const char term_switch_mouse_event_on[]            = "\x1b[?1000h";
static const char term_switch_mouse_tracking_on[]         = "\x1b[?1002h";
static const char term_switch_mouse_tracking_off[]        = "\x1b[?1002l";
static const char term_switch_mouse_event_off[]           = "\x1b[?1000l";
//static const char term_switch_focus_event_on[]            = "\x1b[?1004h";
//static const char term_switch_focus_event_off[]           = "\x1b[?1004l";

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

struct termios termios_initial = {};

void term_restore()
{
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios_initial);
	//fprintf(stdout, term_switch_focus_event_off);
	fprintf(stdout, term_switch_mouse_tracking_off);
	fprintf(stdout, term_switch_mouse_event_off);
	fprintf(stdout, term_switch_mainscreen);
	fprintf(stdout, term_cursor_restore);
}

void term_raw()
{
	int z;
	z = tcgetattr(STDIN_FILENO, &termios_initial);
	if (z < 0) {
		// TODO: use fail function
		errno = ENOTTY;
		perror("term_raw: tcgetattr() failed");
		exit(1);
	}
	atexit(term_restore);

	fprintf(stdout, term_cursor_save);
	fprintf(stdout, term_switch_offscreen);
	fprintf(stdout, term_switch_mouse_event_on);
	fprintf(stdout, term_switch_mouse_tracking_on);
	//fprintf(stdout, term_switch_focus_event_on); // TODO: turn on and check if files need reload.

	struct termios termios_raw = termios_initial;
	termios_raw.c_iflag &= ~BRKINT;                    // no break
	termios_raw.c_iflag &= ~ICRNL;                     // no CR to NL
	termios_raw.c_iflag &= ~INPCK;                     // no parity check
	termios_raw.c_iflag &= ~ISTRIP;                    // no strip character
	termios_raw.c_iflag &= ~IXON;                      // no CR to NL
	termios_raw.c_oflag &= ~OPOST;                     // no post processing
	termios_raw.c_lflag &= ~ECHO;                      // no echo
	termios_raw.c_lflag &= ~ICANON;
	termios_raw.c_lflag &= ~ISIG;
	termios_raw.c_cc[VMIN] = 0;                        // return each byte, or nothing when timeout
	termios_raw.c_cc[VTIME] = 100;                     // 100 * 100 ms timeout
	termios_raw.c_cflag |= CS8;                        // 8 bits chars

	z = tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios_raw);
	if (z < 0) {
		errno = ENOTTY;
		perror("term_raw: tcsetattr() failed");
		exit(1);
	}
}




/********
 * MAIN *
 ********/


int main(int argc, char **args)
{
	logm("enter");
	for (int i = 0; i < argc; i++) {
		puts(args[i]);
		logm("this is a log:%s", args[i]);
		logs(args[i]);
	}

	logm("exit");


	//__assert1(2 < 1);
	__fail_if1(1 < 4);
}
