//#include <assert.h>
//#include <math.h>
//#include <stdarg.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//
//#include <sys/mman.h>
//#include <sys/stat.h>
//
//#include <chi/geometry.h>
//#include <chi/memory.h>
//#include <chi/error.h>
//#include <chi/io.h>

#include <chi/base.h>
#include <chi/log.h>
#include <chi/term.h>
#include <chi/config.h>
#include <chi/error.h>

void blo() { backtrace_print(); }
void bli() { blo(); }
void bla() { bli(); }

int main(int argc, char **args) {
	log_init();

	read_binary(*args);

	logm("enter");

	test_kv_load();

	puts("print backtrace:");
	bla();

	if (1) {
		return 0;
	}

//	term_init();

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
