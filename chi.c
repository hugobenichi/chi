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


/*********
 * UTILS *
 *********/


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
#define __fatal(format, ...) fatal(__LOC__ " %s [ " format, __func__, ##__VA_ARGS__)

// Check if condition is false or true respectively, otherwise print formatted message and exit.
#define __fail_if2(condition, format, ...) if (condition) fatal(__LOC__ " %s [ " format, __func__, ##__VA_ARGS__)
#define __assert2(condition, format, ...) if (!(condition)) fatal(__LOC__ " %s [ " format, __func__, ##__VA_ARGS__)
// Same, but print the failed condition instead of a formatted message.
#define __assert1(condition) __assert2(condition, "assert failed: \"" __stringize2(condition) "\"")
#define __fail_if(condition) __fail_if2(condition, "fatal condition: \"" __stringize2(condition) "\"")
// TODO: replace strerror with just a mapping of errno values to errno symbols like ENOPERM
#define __efail_if(condition) __fail_if2(condition, "fatal condition: \"" __stringize2(condition) "\" failed with: %s", strerror(errno))


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
static const char term_clear[]                            = "\x1bc";
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

struct mapped_file {
	char name[256];
	struct stat file_stat;
	struct slice data;
};

int mapped_file_load(struct mapped_file *f, const char* path)
{
	memcpy(&f->name, path, _arraylen(f->name));

	int fd = open(f->name, O_RDONLY);
	__efail_if(fd < 0);

	int r = fstat(fd, &f->file_stat);
	__efail_if(r < 0);

	int prot = PROT_READ;
	int flags = MAP_SHARED;
	int offset = 0;

	size_t len = f->file_stat.st_size;
	f->data.start = (u8*) mmap(NULL, len, prot, flags, fd, offset);
	f->data.stop  = f->data.start + len;
	__efail_if(f->data.start == MAP_FAILED);

	close(fd);
	return 0;
}

struct configuration {
	int argument_a;
	int argument_b;
};

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

		const char* k = slice_to_string(kv.key);
		const char* v = slice_to_string(kv.val);
		if (slice_empty(kv.val)) {
			printf("W: no val for key '%s'\n", k);
		} else {
			printf("key:%s val:%s\n", k, v);
		}
	}
}

/********
 * MAIN *
 ********/


int main(int argc, char **args)
{
	logm("enter");

	term_raw();

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
