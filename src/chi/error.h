#ifndef __chi_error__
#define __chi_error__

static int TRACE_BUFFER_MAX = 256;

static inline void backtrace_print()
{
  void* trace_buffer[TRACE_BUFFER_MAX];
  int depth = backtrace(trace_buffer, TRACE_BUFFER_MAX);

  char **trace_symbols = backtrace_symbols(trace_buffer, depth);
  for (int i = 0; i < depth; i++) {
    puts(trace_symbols[i]);
  }
  free(trace_symbols);
}

static inline void read_binary(const char *path)
{
	struct mapped_file binary = {};
	mapped_file_load(&binary, path);

  // TODO: parse ELF symbols
}

#endif
