#ifndef __chi_io__
#define __chi_io__

#include <errno.h>
#include <fcntl.h>

struct mapped_file {
	char name[256];
	struct stat file_stat;
	struct slice data;
};

static inline int mapped_file_load(struct mapped_file *f, const char* path)
{
	// TODO: return error if path is too long

	size_t name_maxlen = _arraylen(f->name);
	memset(f->name, 0, name_maxlen);
	memcpy(f->name, path, strnlen(path, name_maxlen - 1) + 1);

	int fd = open(f->name, O_RDONLY);
	__efail_if2(fd < 0, "failed to open %s", f->name);
	// TODO: return error instead

	int r = fstat(fd, &f->file_stat);
	__efail_if2(r < 0, "failed to stat %s", f->name);
	// TODO: return error instead

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


#endif
