#ifndef __chi_base__
#define __chi_base__

#include <errno.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef char      c8;


#define Kilo(x) (1024L * (x))
#define Mega(x) (1024L * Kilo(x))
#define Giga(x) (1024L * Mega(x))

#define __stringize1(x) #x
#define __stringize2(x) __stringize1(x)
#define __LOC__ __FILE__ ":" __stringize2(__LINE__)

// Careful: the result expr gets evaluated twice
#define max(x,y) ((x) > (y) ? (x) : (y))
#define min(x,y) ((x) < (y) ? (x) : (y))

#define _arraylen(ary)    (sizeof(ary)/sizeof(ary[0]))

// Find statically the size of a __VA_ARGS__ list passed to a macroi.
#define NUMARGS(...)  (sizeof((int[]){__VA_ARGS__})/sizeof(int))



// Error and asserts
//
// tl;dr:
//
// fatal + code snippet capture -> __fail_if / __assert1
// fatal + formatted message    -> __fail_if2 / __assert2
// fatal lib call with errno + code snippet capture -> __efail_if
// fatal lib call with errno + formatted message -> __efail_if2

// Print formatted error msg and exit.
static inline void fatal(const char * format, ...)
{
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        fprintf(stderr, "\n");
        exit(1);
}
#define __fatal(format, ...) fatal(__LOC__ " %s [ " format, __func__, ##__VA_ARGS__)

// Check if condition is false or true respectively, otherwise print formatted message and exit.
#define __fail_if2(condition, format, ...) if (condition) fatal(__LOC__ " %s [ " format, __func__, ##__VA_ARGS__)
#define __assert2(condition, format, ...) if (!(condition)) fatal(__LOC__ " %s [ " format, __func__, ##__VA_ARGS__)

// Same, but print the code expression of the failed condition instead of a formatted message.
#define __assert1(condition) __assert2(condition, "assert failed: \"" __stringize2(condition) "\"")
#define __fail_if(condition) __fail_if2(condition, "fatal condition: \"" __stringize2(condition) "\"")

// Same, but print in addition the current errno message.[
// TODO: replace strerror with just a mapping of errno values to errno symbols like ENOPERM
#define __efail_if(condition) __fail_if2(condition, "fatal condition: \"" __stringize2(condition) "\" failed with: %s", strerror(errno))
#define __efail_if2(condition, format, ...) if (condition) fatal(__LOC__ " %s [ %s: " format, __func__, strerror(errno), ##__VA_ARGS__)



enum err_code {
  CHI_ERR_GENERIC = 1000,
};

struct err {
  union {
    int errno_val;
    int is_error;
  };
  const char *loc;
  const char *func;
  struct err *cause;
};

static inline struct err err_new_proto(struct err err)
{
  if (err.cause) {
    struct err *cause_copy = malloc(sizeof(struct err));
    memcpy(cause_copy, err.cause, sizeof(struct err));
    err.errno_val = err.cause->errno_val;
    err.cause = cause_copy;
  }
  return err;
}

static inline void err_acknoledge(struct err err)
{
  struct err *e = err.cause;
  while (e) {
    struct err *f = e;
    e = e->cause;
    free(f);
  }
}

static inline const char* error_msg(struct err err)
{
    switch (err.errno_val) {
    case 0:                           return "Success";
    // TODO: cover all normal errno values
    case EINVAL:                      return "EINVAL";
    case CHI_ERR_GENERIC:             return "Chi generic error";
    default:                          return "Unknown error";
    }
}

#define error(...) err_new_proto((struct err){ .errno_val=CHI_ERR_GENERIC, .loc=__LOC__, .func=__func__, ##__VA_ARGS__})
#define error_because(errno_v) error(.errno_val=errno_v);
#define error_wrap(err) error(.cause=&err);
#define return_if_error(err) if (err.is_error) return error_wrap(err)
#define noerror() error_because(0)

#endif
