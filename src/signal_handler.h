#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

void init_signal_handlers(Config *config);
void should_shutdown(void);
bool should_toggle_pause(void);
void reset_pause_toggle(void);

#endif