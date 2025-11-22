#include "../include/curlsync.h"
#include "config_parser.h"
#include "downloader.h"
#include "logger.h"
#include "signal_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static void print_usage(const char *prog_name) {
    printf("Usage: %s [OPTIONS]\n", prog_name);
    printf("\nOptions:\n");
    printf(" -c, --config FILE  Configuration file (default: config/urls.config)\n");
    printf(" -h, --help         Show this help message\n");
    printf(" -v, --version      Show version information\n");
    printf("\nSignals:\n");
    printf(" Ctrl+C (once)      Pause/Resume downloads\n");
    printf(" Ctrl+C (twice)     Stop and exit\n");
    printf(" SIGTERM            Graceful shutdown\n");
}

static void print_version(void) {
    printf("CurlSync v1.0.0\n");
    printf("A multi-source parallel file downloader using libcurl\n");
}

int main(int argc, char *argv[]) {
    const char *config_file = "config/urls.config";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) {
            if (i + 1 < argc) {
                config_file = argv[++i];
            } else {
                fprintf(stderr, "Error: -c requires a filename\n");
                print_usage(argv[0]);
                return 1;
            }
        } else {
            fprintf(stderr, "Error: Unknown option:%s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    Config config;
    init_config(&config);

    // Initialize logger
    mkdir("logs", 0755);
    if (init_logger(config.log_file) != 0) {
        fprintf(stderr, "Warning: Failed to initialize logger\n");
    }

    log_info("=== CurlSync Starting ===");
    log_info("Using configuration file: %s", config_file);

    // Parse config
    if (parse_config_file(config_file, &config) != SUCCESS) {
        fprintf(stderr, "Error: Failed to parse configuration file\n");
        close_logger();
        return 1;
    }

    if (config.download_count == 0) {
        fprintf(stderr, "Error: No URLs found in configuration file\n");
        close_logger();
        return 1;
    }

    // Create download dir
    mkdir(config.download_dir, 0755);

    // Initialize signal handlers
    init_signal_handlers(&config);

    // Initialize downloader
    if (init_downloader() != SUCCESS) {
        fprintf(stderr, "Error: Failed to initialize downloader\n");
        cleanup_config(&config);
        close_logger();
        return 1;
    }

    printf("CurlSync - Multi-source file downloader\n");
    printf("=========================================\n");
    printf("Configuration:\n");
    printf(" Max parallel downloads: %d\n", config.max_parallel_downloads);
    printf(" Rate limit: %s\n", config.rate_limit_kbps > 0 ?
           "Enabled" : "Disabled");
    printf(" Resume downloads: %s\n", config.resume_downloads ? "Yes" : "No");
    printf(" Verify checksums: %s\n", config.verify_checksums ? "Yes" : "No");
    printf(" Total URLs: %d\n\n", config.download_count);

    // Process downloads
    int result = process_downloads(&config);

    cleanup_downloader();
    cleanup_config(&config);

    log_info("=== CurlSync Finished ===");
    close_logger();

    return (result == SUCCESS) ? 0 : 1;
}