#include <chi/io.h>

#include <errno.h>
#include <stdio.h>
//#include <string.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

static size_t strnlen_polyfill(const char *str, size_t maxlen)
{
  size_t i = 0;
  while (i < maxlen && str[i]) {
    i++;
  }
  return i;
}

int mapped_file_load(struct mapped_file *f, const char *path)
{
	// TODO: return error if path is too long
	size_t maxlen = _arraylen(f->name);
	memset(f->name, 0, maxlen);
	maxlen = strnlen_polyfill(path, maxlen - 1) + 1;
	memcpy(f->name, path, maxlen);

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
