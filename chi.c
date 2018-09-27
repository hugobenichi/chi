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

// TODO: define a fail() function

// Check that given boolean is true, otherwise print format string and exit
void fail_at_location_if(int has_failed, const char *loc, const char * msg, ...)
{
	if (!has_failed) {
		return;
	}
	// TODO: specify a fd instead of stderr
	fprintf(stderr, "%s[ ", loc);
        va_list args;
        va_start(args, msg);
        vfprintf(stderr, msg, args);
        va_end(args);
        fprintf(stderr, "\n");
        exit(1);
}
#define __fail_if(has_failed, format, ...) fail_at_location_if(has_failed, __LOC__ " %s [ " format, __func__, ##__VA_ARGS__)

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

	assert(r < s);
	buffer[r] = '\n';
	return write(LOG_FILENO, buffer, r + 1);
}

#define logm(format, ...)  log_formatted_message(__LOC__ " %s [ " format, __func__, ##__VA_ARGS__)
#define logs(s)  logm("%s", s)



/***********************
 * GEOMETRY PRIMITIVES *
 ***********************/


struct vec {
	i32 x;
	i32 y;
};
typedef struct vec vec;

struct rec {
	union {
		struct {
			vec min;
			vec max;
		};
		struct {
			i32 x0;
			i32 y0;
			i32 x1;
			i32 y1;
		};
	};
};
typedef struct rec rec;

static inline vec v(i32 x ,i32 y)
{
	return (vec){
		.x = x,
		.y = y,
	};
}

static inline rec r(vec min, vec max)
{
	return (rec){
		.min = min,
		.max = max,
	};
}

static inline vec sub_vec_vec(vec v0, vec v1)
{
	return v(v0.x - v1.x, v0.y - v1.y);
}

static inline vec rec_diag(rec r)
{
	return sub_vec_vec(r.max, r.min);
}

static inline i32 rec_w(rec r)
{
	return r.x1 - r.x0;
}

static inline i32 rec_h(rec r)
{
	return r.y1 - r.y0;
}

static inline vec add_vec_vec(vec v0, vec v1)
{
	return v(v0.x + v1.x, v0.y + v1.y);
}

static inline rec add_vec_rec(vec v0, rec r1)
{
	return r(add_vec_vec(v0, r1.min), add_vec_vec(v0, r1.max));
}

static inline rec add_rec_vec(rec r0, vec v1)
{
	return add_vec_rec(v1, r0);
}

static inline rec add_rec_rec(rec r0, rec r1)
{
	return r(add_vec_vec(r0.min, r1.min), add_vec_vec(r0.max, r1.max));
}

static inline rec sub_rec_vec(rec r0, vec v1)
{
	return r(sub_vec_vec(r0.min, v1), sub_vec_vec(r0.max, v1));
}

// Generic adder for vec and rec.
// TODO: support rec addition with a Minkowski sum + displacement ?
#define add(a,b) _Generic((a),          \
	rec:	add_rec_vec,            \
	vec:	add_vec_other(b))((a),(b))

#define add_vec_other(b) _Generic((b),  \
	rec:	add_vec_rec,            \
	vec:	add_vec_vec)

#define sub(a,b) _Generic((a),          \
	rec:	sub_rec_vec,            \
	vec:	sub_vec_vec)((a),(b))


char* vec_print(char *dst, size_t len, vec v)
{
	int n = snprintf(dst, len, "{.x=%d, .y=%d}", v.x, v.y);
	return dst + n; //_min(n, len);
}

char* rec_print(char *dst, size_t len, rec r)
{
	i32 w = rec_w(r);
	i32 h = rec_h(r);
	int n = snprintf(dst, len, "{.x0=%d, .y0=%d, .x1=%d, .y1=%d, w=%d, .h=%d}", r.x0, r.y0, r.x1, r.y1, w, h);
	return dst + n; //_min(n, len);
}


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
}
