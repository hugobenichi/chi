#ifndef __chi_base__
#define __chi_base__

#include <stdint.h>
typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;


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




// Print formatted error msg and exit.
void fatal(const char * format, ...)
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

// Same, but print the failed condition instead of a formatted message.
#define __assert1(condition) __assert2(condition, "assert failed: \"" __stringize2(condition) "\"")
#define __fail_if(condition) __fail_if2(condition, "fatal condition: \"" __stringize2(condition) "\"")

// Same, but print in addition the current errno message.
// TODO: replace strerror with just a mapping of errno values to errno symbols like ENOPERM
#define __efail_if(condition) __fail_if2(condition, "fatal condition: \"" __stringize2(condition) "\" failed with: %s", strerror(errno))
#define __efail_if2(condition, format, ...) if (condition) fatal(__LOC__ " %s [ %s: " format, __func__, strerror(errno), ##__VA_ARGS__)


// TODO: additional macros that create error msg strings for function that don't want to fatal exit.


#endif
