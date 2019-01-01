#ifndef __chi_input__
#define __chi_input__

enum input_type {
  INPUT_NONE,
  INPUT_KEY,
  INPUT_MOUSE,
  INPUT_RESIZE,
  INPUT_UNKNOWN,
};

struct input {
  enum input_type type;
};

#endif
