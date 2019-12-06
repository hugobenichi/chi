#include <errno.h>
#include <assert.h>
#include <dirent.h>
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

#define realloc(ptr, size) debug_realloc(__LOC__, __func__, ptr, size)
#if DEBUG
#define malloc(size) debug_malloc(__LOC__, __func__, size)
#endif

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
	*capacity = *capacity * 8;
	if (*capacity == 0)
		*capacity = 16;
	*array = realloc(*array, stride * *capacity);
	return *array != NULL;
}

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
	*dst = malloc(*dstlen + 1 /* null byte */);
  debug("copy_str(%p, %s)\n", *dst, src);
	memcpy(*dst, src, *dstlen);
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
	size_t size = 1;
	size_t capacity = 1 << 4;
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
	strcpy(buffer, entries->name);
	size_t parent = 0;

	while (d) {
  debug("readdir %s\n", buffer);
		errno = 0;
		struct dirent* entry = readdir(d);

		if (!entry) {
			if (errno != 0) {
				// FIXME: report errno ?
				perror("readdir failed");
				continue;
			}

  debug("closing %s\n", buffer);
			closedir(d);
			d = NULL;
			while (!d && parent < size) {
				parent++;
				if (entries[parent].d_type != DT_DIR) {
					continue;
				}
				index_copy_complete_name(buffer, entries, parent);
  debug("opening %s\n", buffer);
				d = opendir(buffer);
				if (!d) {
					// FIXME: report errno ?
					perror("opendir failed");
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

  debug("%s: read entry %s\n", buffer, entry->d_name);

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

struct navigator {
	struct index_list {
		struct index_list* tail;
		struct index head;
	} *index_list;
	// TODO: currently opened files
};

enum index_error navigator_addindex(struct navigator* navigator, const char* root)
{
	struct index_list **index_list = &navigator->index_list;
	size_t rootlen = strnlen(root, 64);
	while (*index_list) {
		if (strncmp(index_root((*index_list)->head), root, rootlen) == 0) {
			break;
		}
		index_list = &(*index_list)->tail;
	}

	if (*index_list) {
		index_free(&(*index_list)->head);
	} else {
		*index_list = malloc(sizeof(struct index_list));
		if (!*index_list)
			return index_error_enomem;
		(*index_list)->tail = NULL;
	}

	enum index_error e = index_make(&(*index_list)->head, root);
	// FIXME: if error, dealloc e and drop it ?
	return e;
}

void navigator_free(struct navigator* navigator)
{
	struct index_list* index_list = navigator->index_list;
	while (index_list) {
		index_free(&index_list->head);
		struct index_list* current = index_list;
		index_list = index_list->tail;
		free(current);
	}
}

int main(int argc, char* argv[])
{
	if (argc < 2)
		return 0;

	struct navigator navigator;
	memset(&navigator, 0, sizeof(navigator));
	struct index index;
	//enum index_error r = index_make(&index, argv[1]);
	enum index_error r = navigator_addindex(&navigator, argv[1]);

	switch (r) {
		case index_error_invalid_root:
		printf("invalid dir \"%s\"\n", argv[1]);
		return -1;
	case index_error_enomem:
		printf("enomem\n");
		return -1;
	}

	index = navigator.index_list->head;

	char buffer[256];
//	for (int i = 0; i < index.size; i++) {
//		struct index_entry e = index.entries[i];
//		index_copy_complete_name(buffer, index.entries, i);
//		printf("%s %s %s %lu/%lu\n", buffer, e.name, dtype_name(e.d_type), e.namelen, e.total_namelen);
//		puts(buffer);
//	}

	puts("");
	printf("root: %s\n", index_root(index));
	printf("size:%lu\n", index.size);
	printf("capacity:%lu\n", index.capacity);
	printf("entries:%p\n", index.entries);

	if (argc < 3)
		return 0;

	int* matches = NULL;
	int n_matches = index_find_all_matches(&matches, 0, &index, argv[2], match_anywhere);
	printf("found %d matches\n", n_matches);
	for (int i = 0; i < n_matches; i++) {
		index_copy_complete_name(buffer, index.entries, matches[i]);
		puts(buffer);
	}

	free(matches);
	navigator_free(&navigator);
}
