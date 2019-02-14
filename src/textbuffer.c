#include <chi.h>
#include <errno.h>

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
