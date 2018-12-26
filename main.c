#include <chi/chi.c>

int main(int argc, char **args)
{
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
