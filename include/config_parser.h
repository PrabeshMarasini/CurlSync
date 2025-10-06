#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include "../include/curlsync.h"

int parse_config_file(const char *filename, Config *config);
void init_config(Config *config);
void cleanup_config(Config *config);

#endif