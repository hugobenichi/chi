#ifndef __chi_input__
#define __chi_input__

// TODO: add MOUSE, ERROR, ... into input_key_code and suppress one input_type field in struct input.

enum input_type {
  INPUT_NONE,
  INPUT_KEY,
  INPUT_MOUSE,
  INPUT_RESIZE,
  INPUT_ERROR,
  INPUT_UNKNOWN,
};

enum input_key_code {
  // all normal ascii code are implicitly included in the first 127 values

  INPUT_KEY_ESCAPE_Z          = 1000,
  INPUT_KEY_ARROW_UP          = 1001,
  INPUT_KEY_ARROW_DOWN        = 1002,
  INPUT_KEY_ARROW_RIGHT       = 1003,
  INPUT_KEY_ARROW_LEFT        = 1004,
};

struct input {
  enum input_type type;
  enum input_key_code key;
};

#endif
