#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <stdbool.h>
#include "../include/curlsync.h"

void init_signal_handlers(Config *config);
bool should_shutdown(void);
bool should_toggle_pause(void);
void reset_pause_toggle(void);

#endif