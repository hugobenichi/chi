#include <stdio.h>
#include <assert.h>




#include <fcntl.h>
#include <unistd.h>

// The fd value where logs are emitted. Goes to /dev/null by default.
static const int LOG_FILENO = 77;

int init_log() {
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

int logm(char *msg) {
	char buffer[64];
	int r = snprintf(buffer, 64, "log: %s", msg);
	if (64 <= r) {
		r = 64 - 1;
	}
	assert(r < 64);
	buffer[r] = '\n';
	return write(LOG_FILENO, buffer, r + 1);
}



int main(int argc, char **args)
{
	logm("enter");
	for (int i = 0; i < argc; i++) {
		puts(args[i]);
		logm(args[i]);
	}

	logm("exit");
}
