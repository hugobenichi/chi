#include <stdio.h>
#include <assert.h>



#define S1(x) #x
#define S2(x) S1(x)
#define __LOC__ __FILE__ ":" S2(__LINE__)

#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>

// The fd value where logs are emitted. Goes to /dev/null by default.
static const int LOG_FILENO = 77;
// Default size of the log buffer for string assemlbing
static const int LOG_BUFFER_SIZE = 256;

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
