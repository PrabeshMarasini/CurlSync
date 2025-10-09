#include "progress.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int last_line_count = 0;

void init_progress_display(void) {
    last_line_count = 0;
}

void clear_progress_display(void) {
    for (int i = 0; i < last_line_count; i++) {
        printf("\033[A\033[K]]");
    }
    last_line_count = 0;
}

void format_bytes(size_t bytes, char *buffer, size_t buffer_size) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_idx = 0;
    double size = (double)bytes;

    while (size >= 1024.0 && unit_idx < 4) {
        size /= 1024.0;
        unit_idx++;
    }

    snprintf(buffer, buffer_size, "%.2f %s", size, units[unit_idx]);
}

void format_speed(double bytes_per_sec, char *buffer. size_t buffer_size) {
    format_bytes(size_t)bytes_per_sec, buffer, buffer_size;
    strcat(buffer, "/s");
}

static void draw_progress_bar(double progress, char *buffer, size_t buffer_size) {
    int filled = (int)(progress * PROGRESS_BAR_WIDTH);
    int i;

    buffer[0] = '[';
    for (i = 0; i < PROGRESS_BAR_WIDTH; i++) {
        if (i < filled) {
            buffer[i + 1] = '=';
        } else if (i == filled) {
            buffer[i + 1] = '>';
        } else {
            buffer[i + 1] = ' ';
        }
    }
    buffer[PROGRESS_BAR_WIDTH + 1] = ']';
    buffer[PROGRESS_BAR_WIDTH + 2] = '\0';
}

void update_progress_display(Config *config) {
    if (!isatty(STDOUT_FILENO)) {
        return;
    }

    clear_progress_display();

    int line_count = 0;

    // Display each active download
    for (int i = 0; i < config->download_count; i++) {
        DownloadEntry *entry = &config->downloads[i];

        if (entry->state == DOWNLOAD_PENDING || entry->state == DOWNLOAD_COMPLETED) {
            continue;
        }

        char filename[64];
        const char *last_slash = strrchr(entry->output_path, '/');
        if (last_slash) {
            strncpy(filename, last_slash + 1, sizeof(filename) - 1);
        } else {
            strncpy(filename, entry->output_path, sizeof(filename) - 1);
        }
        filename[sizeof(filename) - 1] = '\0';

        // Turncate filename if it is long
        if (strlen(filename) > 20) {
            filename[17] = '.';
            filename[18] = '.';
            filename[19] = '.';
            filename[20] = '\0';
        }

        double progress = 0.0;
        if (entry->total_size > 0) {
            progress = (double)entry->downloaded_size / entry->total_size;
        }

        char progress_bar[PROGRESS_BAR_WIDTH + 3];
        draw_progress_bar(progress, progress_bar, sizeof(progress_bar));

        char downloaded[32], total[32], speed[32];
        format_bytes(entry->downloaded_size, downloaded, sizeof(downloaded));
        format_bytes(entry->total_size, total, sizeof(speed));

        const char *state_str = "";
        switch (entry->state) {
            case DOWNLOAD_ACTIVE: state_str = "Downloading"; break;
            case DOWNLOAD_PAUSED: state_str = "Paused"; break;
            case DOWNLOAD_VERIFYING: state_str = "Verifying"; break;
            case DOWNLOAD_FAILED: state_str = "Failed"; break;
            default: state_str = "Unknown"; break;
        }

        printf("[%d] %-20s %s %6.2f%% %s/%s @ %s [%s]\n",
            entry->id,
            filename,
            progress_bar,
            progress * 100.0,
            downloaded,
            total,
            speed,
            state_str);

        line_count++;
    }

    // Show global pause status
    if (config->paused) {
        printf("\n*** PAUSED - Press Ctrl+C to resume or quit ***\n");
        line_count += 2;
    }

    last_line_count = line_count;
    fflush(stdout);
}