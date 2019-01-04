#ifndef __chi_textbuffer__
#define __chi_textbuffer__

#include <chi/memory.h>
#include <chi/geometry.h>


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

struct textbuffer {
  struct buffer append_buffer;
  struct line *line_first;
  struct line *line_last;
  struct cursor_chain {
    struct cursor *cursor;
    struct cursor *next_cursor;
  } cursors;
  // TODO: operation buffer
};

// TODO: define all ops

#endif
