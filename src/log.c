#include <chi.h>

#include <assert.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>

// Default size of the log buffer for string assemlbing
static const int LOG_BUFFER_SIZE = 256;

static int is_init = 0;

#define LOG_FILENO 77

// Run this at startup to be sure that fd LOG_FILENO is reserved.
void log_init()
{
	int fd = open("/dev/null", O_WRONLY);
	assert(fd > 0);
	assert(dup2(fd, LOG_FILENO) > 0);

	is_init = 1;
}

int log_formatted_message(const char *format, ...)
{
	assert(is_init);

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
