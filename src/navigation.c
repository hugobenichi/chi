#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define __stringize1(x) #x
#define __stringize2(x) __stringize1(x)
#define __LOC__ __FILE__ ":" __stringize2(__LINE__)
#define if_debug if (DEBUG)
#define debug(fmt, ...) if_debug printf(__LOC__ "#%s(): " fmt, __func__, ##__VA_ARGS__)

#define DEBUG 0

void* debug_malloc(const char* loc, const char* func, size_t size) {
	printf("%s %s: malloc(%lu)\n", loc, func, size);
	return malloc(size);
}

void* debug_realloc(const char* loc, const char* func, void* ptr, size_t size) {
	printf("%s %s: realloc(%p, %lu)\n", loc, func, ptr, size);
	if (1) {
		return realloc(ptr, size);
	} else {
		void* new_ptr = malloc(size);
		if (new_ptr && ptr) {
			memcpy(new_ptr, ptr, size);
			free(ptr);
		}
		return new_ptr;
	}
}

#if DEBUG
#define realloc(ptr, size) debug_realloc(__LOC__, __func__, ptr, size)
#define malloc(size) debug_malloc(__LOC__, __func__, size)
#endif

size_t array_find_capacity(size_t capacity, size_t size)
{
	static unsigned int capacity_scaling = 4;
	static unsigned int min_capacity = 16;
	if (capacity == 0)
		capacity = min_capacity;
	while (capacity <= size)
		capacity = capacity * capacity_scaling;
	return capacity;
}

//#define array_append(array, array_size, obj) memcpy(array + sizeof(array[0]) * *(array_size)++, obj, sizeof(array[0]))
#define array_append(array, array_size, obj) array_append_proto((void*)array, array_size, obj, sizeof(array[0]))
void array_append_proto(char* array, size_t* array_size, void* obj, size_t stride)
{
	memcpy(array + stride * (*array_size)++, obj, stride);
}

#define array_ensure_capacity(array, capacity, size) array_ensure_capacity_proto((void**)(array), sizeof(*((array)[0])), capacity, size)
int array_ensure_capacity_proto(void** array, size_t stride, size_t* capacity, size_t size)
{
	size_t c = array_find_capacity(*capacity, size);
	if (c == *capacity)
		return 1;
	*array = (char*) realloc(*array, stride * c);
	*capacity = c;
	return *array != NULL;
}

struct array {
	size_t size;
	size_t capacity;
	char data[];
};

static struct array empty_array = {
	.size = 0,
	.capacity = 0,
};

struct array* array_base(void* array)
{
	return (struct array*) ((char*) array - offsetof(struct array, data));
}

#define array_size(array)     (array_base(array)->size)
#define array_capacity(array) (array_base(array)->capacity)
#define array_free(array)     free(array_base(array))

int array_pushback_proto(void** array_ptr, void* obj, size_t obj_size)
{
	assert(array_ptr);
	struct array* array = (*array_ptr) ? array_base(*array_ptr) : NULL;
	size_t s = array ? array->size : 0;
	size_t c1 = array ? array->capacity : 0;
	size_t c2 = array_find_capacity(c1, s);

	if (c1 != c2) {
  debug("realloc array of size %lu: %lu -> %lu\n", s, c1, c2);
		array = (struct array*) realloc(array, c2 * obj_size + sizeof(struct array));
		if (!array)
			return 0;
		array->size = s; /* needed for initially empty array */
		array->capacity = c2;
		*array_ptr = array->data;
	}

  debug("append value %u at index %lu\n", *((int*)obj), s);
	memcpy(array->data + obj_size * s, obj, obj_size);
	array->size++;
	return 1;
}
// TODO: how to assert types ? Use cpp templates ? Use typeof ? Use static_assert ?
#define array_pushback(array, obj) array_pushback_proto((void**)array, obj, sizeof(*obj))

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
	default:		return "DT_UNKNOWN";
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

void cstrcpy(char** dst, size_t* dstlen, const char* src, size_t maxn)
{
	size_t len = strnlen(src, maxn);
	char* mem = (char*) malloc(len + 1 /* null byte */);
	memcpy(mem, src, len);
	mem[len] = '\0';
	*dst = mem;
	*dstlen = len;
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

const char* index_root(struct index index)
{
	return index.entries->name;
}

enum match_type {
	match_undefined,
	match_anywhere,
	match_anywhere_ignorecase,
	match_from_start,
};

int is_match(const char* candidate, const char* pattern, enum match_type mt)
{
	switch (mt) {
	case match_anywhere_ignorecase:
		// undefined !!
		//return strcasestr(candidate, pattern) != NULL;
	case match_anywhere:
		return strstr(candidate, pattern) != NULL;
	case match_from_start:
		return strncmp(candidate, pattern, strnlen(pattern, 64)) == 0;
	case match_undefined:
	default:
		return 0;
	}
}

int index_find_all_matches(int** out, size_t outcapacity, struct index* index, const char* pattern, enum match_type mt)
{
	assert(out);
	int outsize = 0;
	struct index_entry* entries = index->entries;
	for (int i = 1 /* skip root */; i < index->size; i++) {
		int j = i;
		while (0 < j && !is_match(entries[j].name, pattern, mt)) {
			j = entries[j].parent;
		}
		if (j <= 0) {
			continue;
		}
		if (!array_ensure_capacity(out, &outcapacity, outsize + 1)) {
			// FIXME: return error properly !
			return 0;
		}
		assert(*out);
		(*out)[outsize++] = i;
	}
	return outsize;
}

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

void index_free(struct index* index)
{
	for (int i = 0; i < index->size; i++) {
		free(index->entries[i].name);
	}
	free(index->entries);
}

enum index_error index_make(struct index* index, const char* root)
{
	enum index_error result = index_error_none;

	size_t size = 1; /* first slot used for root */
	size_t capacity = 0;
	struct index_entry* entries = NULL;
	if (!array_ensure_capacity(&entries, &capacity, size))
		return index_error_enomem;

	// FIXME be sure to unalloc this in all return paths !
	int* dir_entries = NULL;
	int next_dir = 0;

	cstrcpy(&entries->name, &entries->namelen, root, 256);
	// trim any extra '/'
	while (entries->name[entries->namelen - 1] == '/')
		entries->name[--entries->namelen] = '\0';
	entries->total_namelen = entries->namelen;
	entries->parent = -1;
	entries->d_type = DT_DIR;

	DIR* d = opendir(entries->name);
	if (!d) {
		result = index_error_invalid_root;
		return result;
		// FIXME: free resources !
		// goto error;
	}

	char buffer[256];
	strcpy(buffer, entries->name);
	size_t parent = 0;

	while (d) {
  debug("readdir %s\n", buffer);
		errno = 0;
		struct dirent* entry = readdir(d);

		if (!entry) {
			if (errno != 0) {
				// Do not hard fail and try again
				perror("readdir failed");
				continue;
			}

  debug("closing %s\n", buffer);
			closedir(d);
			d = NULL;

			assert(next_dir <= array_size(dir_entries));
			if (next_dir == array_size(dir_entries)) {
				goto done;
			}
			parent = dir_entries[next_dir++];
			assert(parent < size);
			index_copy_complete_name(buffer, entries, parent);
  debug("opening %s\n", buffer);
			d = opendir(buffer);
			if (!d) {
				// Do not hard fail and try the next
				perror("opendir failed");
			}
			continue;
		}

		// Only keep DT_DIR, DT_REG, and DT_LNK
		switch (entry->d_type) {
			case DT_BLK:
			case DT_CHR:
			case DT_FIFO:
			case DT_SOCK:
			case DT_UNKNOWN:
  debug("discarding unwanted UNKNOWN\n");
				continue;
			default:
				break;
		}

		// skip '.' and '..'
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

  debug("%s: pushing entry #%lu %s\n", buffer, size, entry->d_name);

		if (!array_ensure_capacity(&entries, &capacity, size + 1)) {
			result = index_error_enomem;
			return result;
			// FIXME: free resources !
			//goto error;
		}

		cstrcpy(&entries[size].name, &entries[size].namelen, entry->d_name, 128);
		entries[size].total_namelen = entries[size].namelen + entries[parent].total_namelen;
		entries[size].total_namelen += 1; // for '/'
		entries[size].parent = parent;
		entries[size].d_type = entry->d_type;

		if (entry->d_type == DT_DIR) {
			if (!array_pushback(&dir_entries, (int*) &size)) {
				result = index_error_enomem;
				return result;
				// FIXME: free resources !
				//goto error;
			}
		}

		size++;
	}

error:
	free(entries->name);
	free(entries);
done:
	if (dir_entries)
		array_free(dir_entries);
	if (d)
		closedir(d);

	index->capacity = capacity;
	index->entries = entries;
	index->size = size;
	return result;
}

struct index_list {
	struct index_list* tail;
	struct index head;
};

struct navigator {
	struct index_list* index_list;
	// TODO: currently opened files
};

enum index_error navigator_addindex(struct navigator* navigator, const char* root)
{
	struct index_list** list = &navigator->index_list;
	size_t rootlen = strnlen(root, 64);
	while (*list) {
		if (strncmp(index_root((*list)->head), root, rootlen) == 0) {
			break;
		}
		list = &(*list)->tail;
	}

	if (*list) {
		index_free(&(*list)->head);
	} else {
		*list = (struct index_list*) malloc(sizeof(struct index_list));
		if (!*list)
			return index_error_enomem;
		(*list)->tail = NULL;
	}

	enum index_error e = index_make(&(*list)->head, root);
	// FIXME: if error, dealloc e and drop it ?
	return e;
}

void navigator_free(struct navigator* navigator)
{
	struct index_list* list = navigator->index_list;
	while (list) {
		index_free(&list->head);
		struct index_list* current = list;
		list = list->tail;
		free(current);
	}
}

int test(int argc, char** argv)
{
	if (argc < 2)
		return 0;

	struct navigator navigator;
	memset(&navigator, 0, sizeof(navigator));
	struct index index;
	//enum index_error r = index_make(&index, argv[1]);
	enum index_error r = navigator_addindex(&navigator, argv[1]);

	switch (r) {
	case index_error_none:
		break;
	case index_error_invalid_root:
		printf("invalid dir \"%s\"\n", argv[1]);
		return -1;
	case index_error_enomem:
		puts("enomem");
		return -1;
	default:
		puts("unknown error");
	}

	index = navigator.index_list->head;

	printf("%lu entries\n", index.size);

	char buffer[256];

	if (argc < 3) {
		navigator_free(&navigator);
		return 0;
		for (int i = 0; i < index.size; i++) {
			index_copy_complete_name(buffer, index.entries, i);
			puts(buffer);
		}
	}

	int* matches = NULL;
	int n_matches = index_find_all_matches(&matches, 0, &index, argv[2], match_anywhere);
	printf("%d matches\n", n_matches);
	for (int i = 0; i < n_matches; i++) {
		index_copy_complete_name(buffer, index.entries, matches[i]);
		puts(buffer);
//		printf("%s %s %s %lu/%lu\n", buffer, e.name, dtype_name(e.d_type), e.namelen, e.total_namelen);
	}
	free(matches);

	exit:
	navigator_free(&navigator);
	return 0;
}

int main(int argc, char* argv[])
{
	for (int i = 0; i < 1; i++) {
		test(argc, argv);
	}
	return 0;
}
