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

struct err textbufer_op_open(struct textbuffer textbuffer, void* args)
{
	return noerror();
}
struct err textbufer_op_touch(struct textbuffer textbuffer, void* args)
{
	return noerror();
}
struct err textbufer_op_save(struct textbuffer textbuffer, void* args)
{
	return noerror();
}
struct err textbufer_op_close(struct textbuffer textbuffer, void* args)
{
	return noerror();
}

struct err textbuffer_operation(struct textbuffer textbuffer, enum textbuffer_op op, void* args)
{
	switch (op) {
	case TEXTBUFFER_OPEN:
		return textbufer_op_open(textbuffer, args);
	case TEXTBUFFER_TOUCH:
		return textbufer_op_touch(textbuffer, args);
	case TEXTBUFFER_SAVE:
		return textbufer_op_save(textbuffer, args);
	case TEXTBUFFER_CLOSE:
		return textbufer_op_close(textbuffer, args);
	defaul:
		return error_because(ENOSYS);
	}
}
