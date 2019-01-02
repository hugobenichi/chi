#ifndef __chi_input__
#define __chi_input__

#include <chi/geometry.h>

enum input_type {
  INPUT_RESIZE,
  INPUT_KEY,
  INPUT_MOUSE,
  INPUT_ERROR,
  INPUT_UNKNOWN,
};

enum input_code {
  // all normal ascii codes are implicitly included in the first 127 values
  INPUT_RESIZE_CODE           = 1000,
  INPUT_KEY_ESCAPE_Z          = 1001,
  INPUT_KEY_ARROW_UP          = 1002,
  INPUT_KEY_ARROW_DOWN        = 1003,
  INPUT_KEY_ARROW_RIGHT       = 1004,
  INPUT_KEY_ARROW_LEFT        = 1005,
  INPUT_MOUSE_LEFT            = 1006,
  INPUT_MOUSE_MIDDLE          = 1007,
  INPUT_MOUSE_RIGHT           = 1008,
  INPUT_MOUSE_RELEASE         = 1009,
  INPUT_ERROR_CODE            = 1010,
};

struct input {
  enum input_code code;
  union {
    vec mouse_click;
    int errno_value;
  };
};

static inline enum input_type input_type_of(struct input input)
{
  if (input.code <= '\x7f') {
      return INPUT_KEY;
  }
  switch (input.code) {
  case INPUT_RESIZE_CODE:
      return INPUT_RESIZE;
  case INPUT_KEY_ESCAPE_Z:
  case INPUT_KEY_ARROW_UP:
  case INPUT_KEY_ARROW_DOWN:
  case INPUT_KEY_ARROW_RIGHT:
  case INPUT_KEY_ARROW_LEFT:
      return INPUT_KEY;
  case INPUT_MOUSE_LEFT:
  case INPUT_MOUSE_MIDDLE:
  case INPUT_MOUSE_RIGHT:
  case INPUT_MOUSE_RELEASE:
      return INPUT_MOUSE;
  case INPUT_ERROR_CODE:
      return INPUT_ERROR;
  default:
      return INPUT_UNKNOWN;
  }
}

#endif
