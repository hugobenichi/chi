#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#define __stringize1(x) #x
#define __stringize2(x) __stringize1(x)
#define __LOC__ __FILE__ ":" __stringize2(__LINE__)
#define if_debug if (DEBUG)
#define debug(fmt, ...) if_debug printf(__LOC__ "#%s(): " fmt, __func__, ##__VA_ARGS__)

#define DEBUG 0

template <typename T>
void swap(T& a, T& b)
{
	T t = a;
	a = b;
	b = t;
}

template <typename T>
T* mcopy(const T* src, size_t srclen)
{
	T* src_copy = (T*) malloc(srclen);
	memcpy(src_copy, src, srclen);
	return src_copy;
}

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

size_t array_find_capacity(size_t current, size_t needed)
{
	static unsigned int capacity_scaling = 4;
	static unsigned int capacity_min = 16;
	if (current == 0)
		current = capacity_min;
	while (current <= needed)
		current = current * capacity_scaling;
	return current;
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

template <typename T>
struct dynarray {
	size_t size;
	size_t capacity;
	T elements[];

	T& operator[](int i)
	{
		return elements[i];
	}

	T& get(int i)
	{
		return elements[i];
	}

	static size_t bytesize(size_t capacity)
	{
		return sizeof(dynarray<T>) + capacity * sizeof(T);
	}
};

template <typename T>
dynarray<T>* array_make(dynarray<T>* array, size_t capacity)
{
	array = (dynarray<T>*) realloc(array, sizeof(dynarray<T>) + capacity * sizeof(T));
	assert(array);
	array->capacity = capacity;
	return array;
}

template <typename T>
struct slice {
	size_t offset;
	size_t size;
	struct dynarray<T>* array;

	T& operator[](int i)
	{
		assert(i + offset < array->capacity);
		return array->elements[offset + i];
	}

	T& last()
	{
		assert(size > 0);
		assert(offset + size - 1 < array->capacity);
		return array->elements[offset + size - 1];
	}

	T* at(int i)
	{
		assert(i + offset < array->capacity);
		return array->elements + offset + i;
	}

	T* at_last(int i)
	{
		assert(size > 0);
		return at(size = 1);
	}

	slice<T> remove(int i)
	{
		assert(0 <= i);
		assert(i < size);
		slice<T> s = *this;
		s.size--;
		swap(array->elements[offset + i], array->elements[offset + size]);
		return s;
	}

	bool is_empty()
	{
		return size == 0;
	}

	void dealloc()
	{
		if (array)
			free(array);
	}

	slice<T> ensure_capacity(size_t capacity)
	{
		slice<T> s = *this;
		size_t current = s.array ? s.array->capacity : 0;
		if (current < capacity) {
			capacity = array_find_capacity(current, capacity);
			s.array = array_make(s.array, capacity);
		}
		return s;
	}

	static slice<T> reserve(size_t capacity)
	{
		slice<T> s = {};
		s.array = array_make<T>(NULL, capacity);
		return s;
	}
};

template <typename T>
void append(struct dynarray<T>** a_ptr, const T& t)
{
	assert(a_ptr);
	struct dynarray<T>* a = *a_ptr;
	if (!a || a->size == a->capacity) {
		size_t new_size = a ? a->size : 0;
		size_t new_capacity = a ? a->capacity * 2 : 16;
		a = (struct dynarray<T>*) realloc(a, sizeof(struct dynarray<T>) + new_capacity * sizeof(T));
		assert(a);
		a->capacity = new_capacity;
		a->size = new_size;
		*a_ptr = a;
	}
	a->elements[a->size] = t;
	a->size++;
}

template <typename T>
slice<T> append(struct slice<T> s, const T& t)
{
	append(&s.array, t);
	s.size++;
	return s;
}

static struct array empty_array = {
	.size = 0,
	.capacity = 0,
};

struct array* array_base(void* array)
{
	if (!array)
		return NULL;
	return (struct array*) ((char*) array - offsetof(struct array, data));
}

#define array_size(array)     (array_base(array)->size)
#define array_capacity(array) (array_base(array)->capacity)
#define array_free(array)     free(array_base(array))

template <typename T>
int array_pushback(T** array_ptr, T* obj)
{
	assert(array_ptr);
	struct array* array = array_base(*array_ptr);
	size_t s = array ? array->size : 0;
	size_t c1 = array ? array->capacity : 0;
	size_t c2 = array_find_capacity(c1, s);
	size_t obj_size = sizeof(*obj);

	if (c1 != c2) {
  debug("realloc array of size %lu: %lu -> %lu\n", s, c1, c2);
		array = (struct array*) realloc(array, c2 * obj_size + sizeof(struct array));
		if (!array)
			return 0;
		array->size = s; /* needed for initially empty array */
		array->capacity = c2;
		*array_ptr = (T*) array->data;
	}

  debug("append value %u at index %lu\n", *((int*)obj), s);
	memcpy(array->data + obj_size * s, obj, obj_size);
	array->size++;
	return 1;
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
	default:		return "DT_UNKNOWN";
	}
}

struct stringview;

// sgring-like class that "owns" the underlying data. Manipulated as a pointer
// and allocated on the heap. String produces should always return this type.
struct string {
	int length; // char array length, only valid for ASCII
	char cstr[0];

	size_t bytesize()       { return sizeof(struct string) + length + 1; }
	bool eq(string* that)   { return length == that->length && strncmp(cstr, that->cstr, length) == 0; }
	struct string* copy()   { return mcopy(this, bytesize()); }
	// These two functions are ASCII only
	char at(int i)          { assert(0 <= i); assert(i < length); return cstr[i]; }
	char last()	        { assert(0 < length); return cstr[length - 1]; }

	static struct string* make(const char* cstr, size_t len)
	{
		len = strnlen(cstr, len);
		struct string* string = (struct string*) malloc(sizeof(struct string) + len + 1);
		memcpy(string->cstr, cstr, len);
		string->cstr[len] = '\0';
		string->length = len;
		return string;
	}

	// TODO: return string length valid for UTF8
	size_t	len() { return 0; }
};

// string-like class that "borrows" an existing string. Always manipulated as a
// value. Do not store unless the lifecycle of the underlying string is known.
// String consumers should always take as arguments this type.
struct stringview {
	int offset;
	int length;
	struct string* string;

	char* cstr()             { return string->cstr + offset; }
	bool eq(stringview that) { return length == that.length && strncmp(cstr(), that.cstr(), length) == 0; }

	static struct stringview make(struct string* string)
	{
		return (struct stringview) {
			.offset = 0,
			.length = string->length,
			.string = string,
		};
	}
};

stringview view(string* string) { return stringview::make(string); }

void cstrcpy(char** dst, size_t* dstlen, const char* src, size_t maxn)
{
	size_t len = strnlen(src, maxn);
	*dst = mcopy(src, len + 1 /* null byte */);
	*dstlen = len;
	dst[len] = '\0';
}

struct index_entry {
	string*		name_;
	size_t		total_namelen;
	int		parent;
	unsigned char	d_type; // copied from struct dirent.d_type

	stringview name() { return view(name_); }
	size_t namelen() { return name_->length; }
};

struct index {
	slice<struct index_entry> entries;

	size_t size() { return entries.size; }
	struct index_entry& operator[](int i) { return entries[i]; }
	stringview root() { return entries[0].name(); }

	void dealloc()
	{
		for (int i = 0; i < entries.size; i++) {
			free(entries[i].name_);
		}
		entries.dealloc();
	}
};

enum match_type {
	match_undefined,
	match_anywhere,
	match_anywhere_ignorecase,
	match_from_start,
};

int is_match(stringview candidate, stringview pattern, enum match_type mt)
{
	switch (mt) {
	case match_anywhere_ignorecase:
		// undefined !!
		//return strcasestr(candidate, pattern) != NULL;
	case match_anywhere:
		return strstr(candidate.cstr(), pattern.cstr()) != NULL;
	case match_from_start:
		return strncmp(candidate.cstr(), pattern.cstr(), pattern.length) == 0;
	case match_undefined:
	default:
		return 0;
	}
}

slice<int> index_find_all_matches(struct index index, stringview pattern, enum match_type mt)
{
	slice<int> out = {};
	for (int i = 1 /* skip root */; i < index.size(); i++) {
		int j = i;
		while (0 < j && !is_match(index[j].name(), pattern, mt)) {
			j = index[j].parent;
		}
		if (j <= 0) {
			continue;
		}
		out = append(out, i);
	}
	return out;
}

void index_copy_complete_name(char* dst, slice<struct index_entry> entries, int entry)
{
	size_t left = entries[entry].total_namelen;
	dst[left] = '\0';
	while (entry >= 0) {
		struct index_entry e = entries[entry];
		size_t offset = e.total_namelen - e.namelen();
		memcpy(dst + offset, e.name_->cstr, e.namelen());
		left -= e.namelen();
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

enum index_error index_make(struct index* out_index, string* root)
{
	//TODO: make sure root is free'd if en error is returned

	DIR* d = opendir(root->cstr);
	if (!d) {
		return index_error_invalid_root;
	}

	// trim any extra '/'
	while (root->last() == '/')
		root->cstr[--root->length] = '\0';

	debug("root size:%lu, root: %s\n", root->length, root->cstr);

	slice<struct index_entry> entries = slice<struct index_entry>::reserve(10);
	// first slot used for root
	entries.size = 1;
	entries[0].name_ = root;
	entries[0].total_namelen = root->length;
	entries[0].parent = -1;
	entries[0].d_type = DT_DIR;

	// FIXME be sure to unalloc this in all return paths !
	slice<size_t> dir_entries = {};
	int next_dir = 0;

	char buffer[256];
	strcpy(buffer, root->cstr);
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

			if (next_dir == dir_entries.size) {
				goto done;
			}

			parent = dir_entries[next_dir++];
			assert(0 < parent);
			assert(parent < entries.size);

  debug("going to next dir #%lu\n", parent);
			index_copy_complete_name(buffer, entries, parent);
  debug("opening %s\n", buffer);
			d = opendir(buffer);
			if (!d) {
				// Do not hard fail and try the next
				char buffer2[256];
				snprintf(buffer2, 256, "opendir(%s) failed", buffer);
				perror(buffer2);
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

  debug("%s: pushing entry #%lu %s\n", buffer, entries.size, entry->d_name);

		if (entry->d_type == DT_DIR) {
			dir_entries = append(dir_entries, entries.size);
		}
		entries = entries.ensure_capacity(entries.size + 1);
		entries.size++;
		struct index_entry& last = entries.last();
		last.name_ = string::make(entry->d_name, 128);
		last.total_namelen = last.namelen() + entries[parent].total_namelen;
		last.total_namelen += 1; // account for '/'
		last.parent = parent;
		last.d_type = entry->d_type;

  debug("%s: entry #%lu %s namelen:%lu total_namelen:%lu\n", buffer, entries.size, entry->d_name,
		last.namelen(),
		last.total_namelen);
	}

error:
	//free(entries->name);
	//free(entries);
done:
	dir_entries.dealloc();
	if (d)
		closedir(d);

	out_index->entries = entries;
	return index_error_none;
}

struct navigator {
	slice<struct index> index_list;
	// TODO: currently opened files

	// FIXME CHANGE TO VIEW
	void rmindex(stringview root)
	{
		for (int i = 0; i < index_list.size; i++) {
			struct index* index = index_list.at(i);
			if (index->root().eq(root)) {
				index->dealloc();
				index_list = index_list.remove(i);
				return;
			}
		}
	}

	enum index_error addindex(string* root)
	{
		struct index index;
		enum index_error e = index_make(&index, root);
		if (e != index_error_none) {
			return e;
		}
		rmindex(view(root));
		index_list = append(index_list, index);
		return index_error_none;
	}

	void dealloc()
	{
		for (int i = 0; i < index_list.size; i++) {
			index_list[i].dealloc();
		}
		index_list.dealloc();
	}
};

int navigation_manualtest(int argc, char** argv)
{
	if (argc < 2)
		return 0;

	struct navigator navigator;
	memset(&navigator, 0, sizeof(navigator));
	enum index_error r = navigator.addindex(string::make(argv[1], 256));

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

	struct index index = navigator.index_list[0];

	printf("%lu entries\n", index.size() - 1 /* do no count root */);

	char buffer[256];

	if (argc < 3) {
		navigator.dealloc();
		return 0;
		for (int i = 0; i < index.size(); i++) {
			index_copy_complete_name(buffer, index.entries, i);
			puts(buffer);
		}
	}

	string* look_pattern = string::make(argv[2], 128);
	slice<int> matches = index_find_all_matches(index, view(look_pattern), match_anywhere);
	printf("%d matches\n", matches.size);
	for (int i = 0; i < matches.size; i++) {
		index_copy_complete_name(buffer, index.entries, matches[i]);
		puts(buffer);
	}
	matches.dealloc();
	free(look_pattern);

	exit:
	navigator.dealloc();
	return 0;
}

/* expected output:
5 matches
/tmp/chi_nav_test//foo/ff1
/tmp/chi_nav_test//foo/ff2
/tmp/chi_nav_test//bar/ff1
/tmp/chi_nav_test//bar/ff2
/tmp/chi_nav_test//bar/ff3
*/
void navigation_autotest()
{

#define mkdir(path) \
	if (mkdir(path, -1) < 0) { \
		char buffer[256]; \
		sprintf(buffer, __LOC__ " #%s(): mkdir(%s) failed: ", __func__, path); \
		perror(buffer); \
	}

#define rm(path) \
	if (remove(path) < 0) { \
		char buffer[256]; \
		sprintf(buffer, __LOC__ " #%s(): remove(%s) failed: ", __func__, path); \
		perror(buffer); \
	}

#define touch(path) \
	if (int fd = open(path, O_RDONLY | O_CREAT) < 0) { \
		char buffer[256]; \
		sprintf(buffer, __LOC__ " #%s(): open(%s) failed: ", __func__, path); \
		perror(buffer); \
	}

	static const char* test_dirs[] = {
		"/tmp/chi_nav_test/",
		"/tmp/chi_nav_test/foo",
		"/tmp/chi_nav_test/bar",
		"/tmp/chi_nav_test/baz",
		NULL,
	};

	static const char* test_files[] = {
		"/tmp/chi_nav_test/f1",
		"/tmp/chi_nav_test/f2",
		"/tmp/chi_nav_test/foo/ff1",
		"/tmp/chi_nav_test/foo/ff2",
		"/tmp/chi_nav_test/foo/fg1",
		"/tmp/chi_nav_test/bar/ff1",
		"/tmp/chi_nav_test/bar/ff2",
		"/tmp/chi_nav_test/bar/ff3",
		"/tmp/chi_nav_test/bar/fg1",
		NULL,
	};

	const char** d = test_dirs;
	const char** f = test_files;

	while (*d) {
		mkdir(*d++);
	}
	while (*f) {
		touch(*f++);
	}

	struct navigator navigator = {};
	struct index index;
	slice<int> matches;
	string* pattern;

	enum index_error r = navigator.addindex(string::make(test_dirs[0], 256));
	if (r != index_error_none) {
		puts("error");
		goto cleanup;
	}

	index = navigator.index_list[0];
	pattern = string::make("ff", 128);
	matches = index_find_all_matches(index, view(pattern), match_anywhere);

	printf("%d matches\n", matches.size);
	char buffer[256];
	for (int i = 0; i < matches.size; i++) {
		index_copy_complete_name(buffer, index.entries, matches[i]);
		puts(buffer);
	}
	matches.dealloc();
	navigator.dealloc();
	free(pattern);

	cleanup:
	while (f-- != test_files) {
		rm(*f);
	}
	while (d-- != test_dirs) {
		rm(*d);
	}

}

int main(int argc, char* argv[])
{
//	navigation_autotest();
//	return 0;
	for (int i = 0; i < 1; i++) {
		navigation_manualtest(argc, argv);
	}
	return 0;
}
