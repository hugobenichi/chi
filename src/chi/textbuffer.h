#ifndef __chi_textbuffer__
#define __chi_textbuffer__

void textbuffer_init();

// A single line of text, made of a linked list of struct slices.
// Lines are linked together in a doubly-linked lists for simple navigation and insertions.
struct line {
  struct line *prev;
  struct line *next;
  struct textpiece {
    struct textpiece *next;
    struct slice slice;
  } fragment;
  size_t bytelen;
};

// A cursor pointing into a single location into a single line of text.
// It retains a link to its owning textbuffer.
struct cursor {
  struct textbuffer *textbuffer;
  struct line *line;
  // TODO: change to lineno and x_offset ?
  vec position;
};


// this is just an opaque handle
struct textbuffer {
};


// TODO: define all ops
enum textbuffer_op {
  TEXTBUFFER_OPEN,
  TEXTBUFFER_TOUCH,
  TEXTBUFFER_SAVE,
  TEXTBUFFER_CLOSE,

  TEXTBUFFER_MAX,
};

struct textbuffer_command {
  int op;
  int arg_size;
  void* args;
};

struct err textbuffer_operation(struct textbuffer textbuffer, struct textbuffer_command *command);

#endif
