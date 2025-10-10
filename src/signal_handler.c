#include "signal_handler.h"
#include "logger.h"
#include <signal.h>
#include <stdbool.h>

static volatile sig_atomic_t shutdown_requested = 0;
static volatile sig_atomic_t pause_toggle_requested = 0;
static Config *global_config = NULL;

static void sigint_handler(int signum) {
    (void)signum;
    static int sigint_count = 0;
    sigint_count++;

    if (sigint_count == 1) {
        pause_toggle_requested = 1;
        log_info("Pause/Resume requested");
    } else {
        shutdown_requested = 1;
        log_info("Shutdown requested");
    }
}

static void sigterm_handler(int signum) {
    (void)signum;
    shutdown_requested = 1;
    log_info("SIGTERM received, shutting down");
}

void init_signal_handlers(Config *config) {
    global_config = config;

    struct sigaction as_int;
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);

    struct sigaction as_term;
    sa_term.sa_handler = sigterm_handler;
    sigemptyset(&sa_term.sa_mask);
    sa_term.sa_flags = 0;
    sigaction(SIGTERM, &sa_term, NULL);
}

bool should_shutdown(void) {
    return shutdown_requested != 0;
}

bool should_toggle_pause(void) {
    return pause_toggle_requested != 0;
}

void reset_pause_toggle(void) {
    pause_toggle_requested =;
}