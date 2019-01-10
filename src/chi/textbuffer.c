#include <chi/textbuffer.h>
#include <errno.h>

struct textbuffer_impl {
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
