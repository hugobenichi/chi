#include <chi.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
//#include <string.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

int mapped_file_load(struct mapped_file *f, const char *path)
{
	// TODO: return error if path is too long
	size_t maxlen = _arraylen(f->name);
	memset(f->name, 0, maxlen);
	maxlen = strnlen_polyfill(path, maxlen - 1) + 1;
	memcpy(f->name, path, maxlen);

	int fd = open(f->name, O_RDONLY);
	assertf_success(fd < 0, "failed to open %s", f->name);
	// TODO: return error instead

	int r = fstat(fd, &f->file_stat);
	assertf_success(r < 0, "failed to stat %s", f->name);
	// TODO: return error instead

	int prot = PROT_READ;
	int flags = MAP_SHARED;
	int offset = 0;
	size_t len = f->file_stat.st_size;

	f->data.start = mmap(NULL, len, prot, flags, fd, offset);
	f->data.stop  = f->data.start + len;
	assert(f->data.start != MAP_FAILED);

	close(fd);
	return 0;
}
