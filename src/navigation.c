#include <errno.h>
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* dtype_name(unsigned char dtype) {
	switch (dtype) {
	case DT_BLK:		return "DT_BLK";
	case DT_CHR:		return "DT_CHR";
	case DT_DIR:		return "DT_DIR";
	case DT_FIFO:		return "DT_FIFO";
	case DT_LNK:		return "DT_LNK";
	case DT_REG:		return "DT_REG";
	case DT_SOCK:		return "DT_SOCK";
	case DT_UNKNOWN:
	defalut:		return "DT_UNKNOWN";
	}
}

struct string {
  int capacity;
  int length;
  char c_str[0];
};

struct stringview {
  int length;
  const char* c_str;
};

void copy_cstr(char** dst, size_t* dstlen, const char* src, size_t maxn)
{
	*dstlen = strnlen(src, maxn);
	*dst = malloc(*dstlen);
	memcpy(*dst, src, *dstlen);
	if (*dstlen == maxn) {
		(*dstlen)--;
	}
	(*dst)[*dstlen] = '\0';
}

struct stringview string_to_view(const struct string* s)
{
	return (struct stringview) {
		.length = s->length,
		.c_str  = s->c_str,
	};
}

struct index_entry {
	char*		name;
	size_t		namelen;
	size_t		total_namelen;
	int		parent;
	unsigned char	d_type; // copied from struct dirent.d_type
};

struct index {
	struct index_entry*	entries;
	size_t			size;
	size_t			capacity;
};

void index_copy_complete_name(char* dst, struct index_entry* entries, int entry)
{
	size_t left = entries[entry].total_namelen;
	dst[left] = '\0';
	while (entry >= 0) {
		struct index_entry e = entries[entry];
		size_t offset = e.total_namelen - e.namelen;
		memcpy(dst + offset, e.name, e.namelen);
		left -= e.namelen;
		entry = e.parent;
		if (entry >= 0) {
			dst[offset - 1] = '/';
			left--;
		}
	}
	assert(left == 0);
}

enum index_error {
	index_error_none,
	index_error_invalid_root,
	index_error_enomem
};

//#define array_append(array, array_size, obj) memcpy(array + sizeof(array[0]) * *(array_size)++, obj, sizeof(array[0]))
#define array_append(array, array_size, obj) array_append_proto((void*)array, array_size, (void*) obj, sizeof(array[0]))
void array_append_proto(char* array, size_t* array_size, char* obj, size_t stride)
{
	memcpy(array + stride * (*array_size)++, obj, stride);
}

#define array_ensure_capacity(array, capacity, size) array_ensure_capacity_proto(((void*)array), sizeof(*((array)[0])), (capacity), (size))
int array_ensure_capacity_proto(char** array, size_t stride, size_t* capacity, size_t size)
{
	if (size < *capacity)
		return 1;
	*capacity = *capacity * 2;
	*array = realloc(*array, stride * *capacity);
	return *array != NULL;
}

void index_free(struct index* index)
{
	for (int i = 0; i < index->size; i++) {
		free(index->entries[i].name);
	}
	free(index->entries);
}

enum index_error index_make(struct index* index, const char* root)
{
	size_t size = 1;
	size_t capacity = 10;
	struct index_entry* entries = malloc(sizeof(struct index_entry) * capacity);
	if (!entries)
		return index_error_enomem;

	copy_cstr(&entries->name, &entries->namelen, root, 256);
	// trim any extra '/'
	while (entries->name[entries->namelen - 1] == '/')
		entries->name[--entries->namelen] = '\0';
	entries->total_namelen = entries->namelen;
	entries->parent = -1;
	entries->d_type = DT_DIR;

	DIR* d = opendir(entries->name);
	if (!d) {
		free(entries->name);
		free(entries);
		return index_error_invalid_root;
	}

	char buffer[256];
	size_t parent = 0;
	while (d) {
		errno = 0;
		struct dirent* entry = readdir(d);
		if (!entry) {
			// FIXME: check errno to distinguis from end of stream
			// find next directory and continue
			closedir(d);
			d = NULL;
			parent++;
			for (; parent < size; parent++) {
				if (entries[parent].d_type != DT_DIR) {
					continue;
				}
				index_copy_complete_name(buffer, entries, parent);
				puts("opening !");
				puts(buffer);
				d = opendir(buffer);
				if (!d) {
					// FIXME: report errno ?
					perror("opendir failed");
					break;
				}
			}
			continue;
		}

		// skip '.' and '..'
		const char* n = entry->d_name;
		if (n[0] == '.' && n[1] == '\0')
			continue;
		if (n[0] == '.' && n[1] == '.' && n[2] == '\0')
			continue;

		if (!array_ensure_capacity(&entries, &capacity, size))
			return index_error_enomem;

		copy_cstr(&entries[size].name, &entries[size].namelen, entry->d_name, 256);
		entries[size].total_namelen = entries[size].namelen + entries[parent].total_namelen;
		entries[size].total_namelen += 1; // for '/'
		entries[size].parent = parent;
		entries[size].d_type = entry->d_type;
		size++;
	}

	assert(!d);

	index->capacity = capacity;
	index->entries = entries;
	index->size = size;
	return index_error_none;
}

int main(int argc, char* argv[])
{
	if (argc < 2)
		return 0;

	struct index index;
	enum index_error r = index_make(&index, argv[1]);

	switch (r) {
		case index_error_invalid_root:
		printf("invalid dir \"%s\"\n", argv[1]);
		return -1;
	case index_error_enomem:
		printf("enomem\n");
		return -1;
	}

	char buffer[256];
	for (int i = 0; i < index.size; i++) {
		struct index_entry e = index.entries[i];
		index_copy_complete_name(buffer, index.entries, i);
		printf("%s %s %s %lu/%lu\n", buffer, e.name, dtype_name(e.d_type), e.namelen, e.total_namelen);
	}

	puts("");
	printf("rootlen: %lu\n", index.entries->namelen);
	printf("size:%lu\n", index.size);
	printf("capacity:%lu\n", index.capacity);
	printf("entries:%p\n", index.entries);

	return 0;
}
