#include <errno.h>
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct string {
  int capacity;
  int length;
  char c_str[0];
};

struct stringview {
  int length;
  const char* c_str;
};

size_t copy_cstr(char** dst, const char* src, size_t maxn)
{
	size_t len = strnlen(src, maxn);
	*dst = malloc(len);
	memcpy(*dst, src, len);
	if (len == maxn) {
		(*dst)[len - 1] = '\0';
		len--;
	}
	return len;
}

struct stringview string_to_view(const struct string* s)
{
	return (struct stringview) {
		.length = s->length,
		.c_str  = s->c_str,
	};
}

struct index_entry {
	unsigned char dtype;
	size_t namelen;
	char* name;
};

struct index {
	struct dirent* entries;
	size_t size;
	size_t capacity;
	char	root[256];
};

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

enum index_error file_index_make(struct index* index)
{
	DIR* d = opendir(index->root);
	if (!d)
		return index_error_invalid_root;

	size_t size = 0;
	size_t capacity = 10;
	struct dirent* entries = malloc(sizeof(struct dirent) * capacity);
	if (!entries) {
		closedir(d);
		return index_error_enomem;
	}

	size_t nextdir = 0;
	while (d) {
		errno = 0;
		struct dirent* entry = readdir(d);
		if (!entry) {
			// FIXME: check errno to distinguis from end of stream
			// find next directory and continue
			closedir(d);
			d = NULL;
			for (; nextdir < size; nextdir++) {
				if (entries[nextdir].d_type == DT_DIR) {
					// FIXME need to link to parent !!!
					d = opendir(entries[nextdir].d_name);
					if (!d)
						break;
				}
			}
			continue;
		}

		printf("%s %u\n", entry->d_name, entry->d_reclen);

		if (!array_ensure_capacity(&entries, &capacity, size))
			return index_error_enomem;
		array_append(entries, &size, entry);
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

	puts(argv[1]);

	struct index index;
	strncpy(index.root, argv[1], sizeof(index.root));

	enum index_error r = file_index_make(&index);

	switch (r) {
		case index_error_invalid_root:
		printf("invalid dir \"%s\"\n", index.root);
		return -1;
	case index_error_enomem:
		printf("enomem\n");
		return -1;
	}

	printf("size:%lu\n", index.size);
	printf("capacity:%lu\n", index.capacity);
	printf("entries:%p\n", index.entries);

	for (int i = 0; i < index.size; i++) {
		//puts(index.entries[i].d_name);
	}

	return 0;
}
