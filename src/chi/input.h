#ifndef __chi_input__
#define __chi_input__

enum input_type {
  INPUT_RESIZE,
  INPUT_KEY,
  INPUT_MOUSE,
  INPUT_ERROR,
  INPUT_UNKNOWN,
};

enum input_code {
  // all ascii control codes
  CTRL_AT,
  CTRL_A,
  CTRL_B,
  CTRL_C,
  CTRL_D,
  CTRL_E,
  CTRL_F,
  CTRL_G,
  CTRL_H,
  CTRL_I,
  CTRL_J,
  CTRL_K,
  CTRL_L,
  CTRL_M,
  CTRL_N,
  CTRL_O,
  CTRL_P,
  CTRL_Q,
  CTRL_R,
  CTRL_S,
  CTRL_T,
  CTRL_U,
  CTRL_V,
  CTRL_W,
  CTRL_X,
  CTRL_Y,
  CTRL_Z,
  CTRL_LEFT_BRACKET,
  CTRL_BACKSLASH,
  CTRL_RIGHT_BRACKET,
  CTRL_CARET,
  CTRL_UNDERSCORE,

  // first printable ascii code
  SPACE,
  // all other printable ascii codes implicitly included here.

  // last ascii code.
  DEL                         = '\x7f',

  // additional special codes representing other input sequences.
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

// Other "famous" keys mapping to control codes
static const int ESC                   = CTRL_LEFT_BRACKET;
static const int BACKSPACE             = CTRL_H;
static const int TAB                   = CTRL_I;
static const int LINE_FEED             = CTRL_J;
static const int VTAB                  = CTRL_K;
static const int NEW_PAGE              = CTRL_L;
static const int ENTER                 = CTRL_M;

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

static inline int is_printable_key(int code)
{
  return ' ' <= code && code <= '~';
}

#endif
