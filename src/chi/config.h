#ifndef __chi_config__
#define __chi_config__

struct config {
  int debug_noterm;
  int debug_config;
};

struct config CONFIG;

void config_init();


#endif
