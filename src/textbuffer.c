#include <chi.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#define textchunk_size (0x8000 + sizeof(struct textchunk)) // 32k text chunk

struct textchunk {
	struct textchunk *next;
	size_t cursor;
	char text[0];
};

static struct textchunk *textchunk_free_list_head = NULL;
static int textchunk_total_count = 0;
static int textchunk_free_count = 0;

struct textchunk* textchunk_alloc()
{
	if_null(textchunk_free_list_head) {
		textchunk_total_count++;
		textchunk_free_count++;
		textchunk_free_list_head = malloc(textchunk_size);
	}
	textchunk_free_count--;
	struct textchunk *chunk = textchunk_free_list_head;
	textchunk_free_list_head = chunk->next;
	chunk->cursor = 0;
	chunk->next = NULL;
	return chunk;
}

void textchunk_free(struct textchunk *chunk)
{
	assert(chunk);
	struct textchunk dummy = { .next = chunk };
	struct textchunk *end = &dummy;
	while (end->next) {
		textchunk_free_count++;
		end = end->next;
	}
	end->next = textchunk_free_list_head;
	textchunk_free_list_head = chunk;
}

struct textbuffer_impl {
  struct textbuffer_impl *next_free_slot;
  struct mapped_file file;
  struct buffer append_buffer;
  struct line *line_first;
  struct line *line_last;
  struct cursor_chain {
    struct cursor *cursor;
    struct cursor *next_cursor;
  } cursors;
  // TODO: operation buffer
};


// Storage of text buffers:
//	Text buffers are stored in a static array of fixed size with constant stride.
//	The slots in that array are chained together by using the first field of struct textbuffer_impl. This also helps identify free slots.
//	When a text buffer is allocated, the first free slot is used, and the chain of free slots is updated.
//	When a text buffer is freed, the slot is returned to the free slot list.
//	When the array is full and more buffers needs to be allocated, resizing needs to reconstruct the linked list.

static struct textbuffer_impl *all_textbuffers = NULL;
static struct textbuffer_impl *all_textbuffers_freelist_next = NULL;
static struct textbuffer_impl *all_textbuffers_freelist_last = NULL;
static size_t all_textbuffers_length = 0;
static size_t all_textbuffers_capacity = 0;

static const int all_textbuffer_length_default = 64;

static void all_textbuffers_close_free_list()
{
	// Write something else than NULL so that this slot counts as free.
	all_textbuffers_freelist_last->next_free_slot = all_textbuffers_freelist_last;
}

static inline struct textbuffer_impl* all_textbuffers_find_first_free_slot(struct textbuffer_impl *textbuffer)
{
	if (textbuffer) {
		while (textbuffer->next_free_slot) {
			textbuffer = textbuffer->next_free_slot;
		}
	}
	return textbuffer;
}

static void all_textbuffers_resize()
{
	int new_capa = all_textbuffers_capacity * 2;
	if (!new_capa) {
		new_capa = all_textbuffer_length_default;
	}
	size_t new_size = new_capa * sizeof(struct textbuffer_impl);

	struct textbuffer_impl *new_storage = realloc(all_textbuffers, new_size);
	if (!all_textbuffers) {
		memset(new_storage, 0, new_size);
		// construct the freelist for the first time
		all_textbuffers_freelist_next = new_storage;
		all_textbuffers_freelist_last = new_storage + new_capa;
		all_textbuffers_close_free_list();
		struct textbuffer_impl *slot = new_storage;
		while (slot < all_textbuffers_freelist_last) {
			slot->next_free_slot = slot + 1;
			slot = slot->next_free_slot;
		}
	} else {
		// reconstruct the freelist
		// Since slots knows if they are allocated or not, we take the
		// opportunity to defrag the list
		all_textbuffers_freelist_next = all_textbuffers_find_first_free_slot(new_storage);
		struct textbuffer_impl *slot = all_textbuffers_freelist_next;
		while (slot < all_textbuffers_freelist_last) {
			slot->next_free_slot = slot + 1;
			slot = slot->next_free_slot;
			// detect end of loop !
		}
		all_textbuffers_freelist_last = new_storage + new_capa;
		all_textbuffers_close_free_list();
	}
	all_textbuffers_capacity = new_capa;
	all_textbuffers = new_storage;
}

static struct textbuffer_impl* all_textbuffer_alloc()
{
	if (all_textbuffers_length == all_textbuffers_capacity) {
		all_textbuffers_resize();
	}
	all_textbuffers_length++;
	struct textbuffer_impl* textbuffer = all_textbuffers_freelist_next;
	all_textbuffers_freelist_next = textbuffer->next_free_slot;
	textbuffer->next_free_slot = NULL;
	return textbuffer;

}

static void all_textbuffers_free(struct textbuffer_impl *textbuffer)
{
	all_textbuffers_length++;
	all_textbuffers_freelist_last->next_free_slot = textbuffer;
	all_textbuffers_freelist_last = textbuffer;
	all_textbuffers_close_free_list();
}

static struct textbuffer_impl* textbuffer_alloc()
{
	return NULL;
}



static struct err textbuffer_op_open(struct textbuffer textbuffer, struct textbuffer_command *command)
{
	return noerror();
}
static struct err textbuffer_op_touch(struct textbuffer textbuffer, struct textbuffer_command *command)
{
	return noerror();
}
static struct err textbuffer_op_save(struct textbuffer textbuffer, struct textbuffer_command *command)
{
	return noerror();
}
static struct err textbuffer_op_close(struct textbuffer textbuffer, struct textbuffer_command *rgs)
{
	return noerror();
}

typedef struct err (*op_handler)(struct textbuffer, struct textbuffer_command*);
static op_handler op_dispatch[TEXTBUFFER_MAX] = {
  [TEXTBUFFER_OPEN]	= textbuffer_op_open,
  [TEXTBUFFER_TOUCH]	= textbuffer_op_touch,
  [TEXTBUFFER_SAVE]	= textbuffer_op_save,
  [TEXTBUFFER_CLOSE]	= textbuffer_op_close,
};

struct err textbuffer_operation(struct textbuffer textbuffer, struct textbuffer_command *command)
{
	int op = command->op;
	if (op < 0 || TEXTBUFFER_MAX <= op) {
		return error_because(ENOSYS);
	}
	return op_dispatch[op](textbuffer, command);
}

void textbuffer_init()
{
	// TODOs:
	// setup memory for storing all textbuffer
	// setup memory for storing an index of textbuffer by name
}

static size_t file_path_maxlen = 1024;

int textbuffer_load(const char *path, struct textbuffer *textbuffer)
{
	assert(path);
	size_t len = strnlen_polyfill(path, file_path_maxlen);
	char* path_copy = malloc(len + 1);
	memcpy(path_copy, path, len + 1);

	// FIXME: need to close FD !
	int fd = open(path_copy, O_RDONLY);
	if (!fd) {
		return -errno;
	}
	struct stat stat;
	int r = fstat(fd, &stat);
	if (r < 0) {
		return -errno;
	}

	textbuffer->path = path_copy;
	textbuffer->basename = path_copy;
	char* last_slash = path_copy;
	while (last_slash) {
		textbuffer->basename = last_slash;
		last_slash = strchr(last_slash, '/');
	}
	textbuffer->basename++;

	// Load file in chunk
	ssize_t filesize = stat.st_size;
	struct textchunk **chunk_emplace = &textbuffer->textchunk_head;
	while (0 < filesize) {
		struct textchunk *chunk = textchunk_alloc();
		*chunk_emplace = chunk;
		chunk_emplace = &chunk->next;
		textbuffer->textchunk_last = chunk;
		if (!chunk) {
			return -ENOMEM;
		}

		ssize_t r = read(fd, chunk->text, textchunk_size);
		if (r < 0) {
			return -errno;
		}
		if (r == 0) {
			assert(filesize == 0);
			// Broken invariant: r should always be <= filesize, and filesize always > 0
			return -EINVAL;
		}
		chunk->cursor += r;
		filesize -= r;
		assert((r == textchunk_size) || (filesize == 0));
	}

	// TODO: cut the file into lines

	// TODO: init one cursor

	return 0;
}
