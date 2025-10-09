#ifndef PROGRESS_H
#define PROGRESS_H

#include "../include/curlsync.h"

void init_progress_display(void);
void update_progress_display(Config *config);
void clear_progress_display(void);
void frmat_bytes(size_t bytes, char *buffer, size_t buffer_size);
void format_speed(double bytes_per_sec, char *buffer, size_t buffer_size);

#endif