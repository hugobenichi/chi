#ifndef __chi_log__
#define __chi_log__

// The fd value where logs are emitted. Goes to /dev/null by default.
const int LOG_FILENO;

void log_init();
int log_formatted_message(const char *format, ...);


#define logm(format, ...)  log_formatted_message(__LOC__ " %s [ " format, __func__, ##__VA_ARGS__)
#define logs(s)  logm("%s", s)

#endif
