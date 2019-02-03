#ifndef __chi_io__
#define __chi_io__

// DOCME
struct mapped_file {
	char name[256];
	struct stat file_stat;
	struct slice data;
};

// DOCME
int mapped_file_load(struct mapped_file *f, const char *path);

#endif
