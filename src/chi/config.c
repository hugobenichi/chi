#include <chi.h>

struct config CONFIG = {
  .debug_noterm = 1,
  .debug_config = 0,
};

static int is_space(u8 c) {
	return c == ' ' || c == '\t';
}

static int is_not_space(u8 c) {
	return !is_space(c);
}

struct keyval {
	struct slice key;
	struct slice val;
};

static struct keyval keyval_from_line(struct slice s)
{
	struct keyval kv;

	s = slice_split(&s, '#');

	slice_while(&s, is_space);
	kv.key = slice_while(&s, is_not_space);

	slice_while(&s, is_space);
	kv.val = slice_while(&s, is_not_space);

	return kv;
}

void config_init()
{
	struct mapped_file config;
	mapped_file_load(&config, "./config.txt");

	while (slice_len(config.data)) {
		struct slice line = slice_take_line(&config.data);
		if (slice_empty(line)) {
			continue;
		}

		struct keyval kv = keyval_from_line(line);
		if (slice_empty(kv.key)) {
			continue;
		}

		char* k = slice_to_string(kv.key);
		char* v = slice_to_string(kv.val);
if (CONFIG.debug_config) {
		if (slice_empty(kv.val)) {
			printf("W: no val for key '%s'\n", k);
		} else {
			printf("key:%s val:%s\n", k, v);
		}
}

		if (k) free(k);
		if (v) free(v);
	}
}
