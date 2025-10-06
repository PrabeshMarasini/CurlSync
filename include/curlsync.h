#ifndef CURLSYNC_H
#define CURLSYNC_H

#include <curl/curl.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_URLS 100
#define MAX_PATH LEN 512
#define MAX_URL_LEN 2048
#define SHA256_DIGEST_LENGTH 32
#define PROGRESS_BAR_WIDTH 32

// Errro codes
#define SUCCESS 0
#define ERROR_CONFIG_PARSE -1
#define ERROR_DOWNLOAD -2
#define ERROR_CHECKSUM -3
#define ERROR_FILE_IO -4

// Download states
typedef enum {
    DOWNLOAD_PENDING,
    DOWNALOD_ACTIVE,
    DOWNLOAD_PAUSED,
    DOWNLOAD_COMPLETED,
    DOWNLAOD_FAILED,
    DOWNLOAD_VERIFYING
} DownloadState;

// Download entry structure
typedef struct {
    char url[MAX_URL_LEN];
    char output_path[MAX_PATH_LEN];
    char sha256[SHA256_DIGEST_LENGTH * 2 + 1];
    size_t total_size;
    size_t downaloded_size;
    DownloadState state;
    CURL *curl_handle;
    FILE *file;
    double download_speed;
    time_t start_time;
    int id;
} DownloadEntry;

// Global config
typedf struct {
    int max_parallel_downlaods;
    int rate_limit_kbps;
    bool verify_checksums;
    bool resume_downloads;
    char log_file[MAX_PATH_LEN];
    char download_dir[MAX_PATH_LEN];
    DownloadEntry downloads[MAX_URLS];
    int download_count;
    bool paused;
} Config;

#endif