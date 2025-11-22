#include "downloader.h"
#include "logger.h"
#include "progress.h"
#include "checksum.h"
#include "rate_limiter.h"
#include "signal_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

static CURLM *multi_handle = NULL;

int init_downloader(void) {
    curl_global_init(CURL_GLOBAL_ALL);
    multi_handle = curl_multi_init();
    if (!multi_handle) {
        log_error("Failed to initialize curl multi handle");
        return ERROR_DOWNLOAD;
    }
    return SUCCESS;
}

void cleanup_downloader(void) {
    if (multi_handle) {
        curl_multi_cleanup(multi_handle);
        multi_handle = NULL;
    }
    curl_global_cleanup();
}

static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    DownloadEntry *entry = (DownloadEntry *)userdata;
    size_t written = fwrite(ptr, size, nmemb, entry->file);
    entry->downloaded_size += written;
    return written;
}

static int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
                            curl_off_t ultotal, curl_off_t ulnow) {
    (void)ultotal;
    (void)ulnow;
    
    DownloadEntry *entry = (DownloadEntry *)clientp;
    entry->total_size = (size_t)dltotal;
    entry->downloaded_size = (size_t)dlnow;
    
    return 0;
}

static long get_file_size(const char *filepath) {
    struct stat st;
    if (stat(filepath, &st) == 0) {
        return st.st_size;
    }
    return 0;
}

static int setup_download(DownloadEntry *entry, Config *config) {
    // Dynamically allocate full path
    size_t path_len = snprintf(NULL, 0, "%s/%s", config->download_dir, entry->output_path) + 1;
    char *full_path = malloc(path_len);
    if (!full_path) {
        log_error("Failed to allocate memory for full path");
        return ERROR_FILE_IO;
    }
    snprintf(full_path, path_len, "%s/%s", config->download_dir, entry->output_path);
    
    // Create directory if needed
    char *last_slash = strrchr(full_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        mkdir(full_path, 0755);
        *last_slash = '/';
    }
    
    long resume_from = 0;
    if (config->resume_downloads) {
        resume_from = get_file_size(full_path);
        if (resume_from > 0) {
            entry->downloaded_size = resume_from;
            log_info("Resuming download from %ld bytes: %s", resume_from, entry->url);
        }
    }
    
    const char *mode = (resume_from > 0) ? "ab" : "wb";
    entry->file = fopen(full_path, mode);
    if (!entry->file) {
        log_error("Failed to open file for writing: %s", full_path);
        free(full_path);
        return ERROR_FILE_IO;
    }
    
    entry->curl_handle = curl_easy_init();
    if (!entry->curl_handle) {
        fclose(entry->file);
        entry->file = NULL;
        free(full_path);
        return ERROR_DOWNLOAD;
    }
    
    curl_easy_setopt(entry->curl_handle, CURLOPT_URL, entry->url);
    curl_easy_setopt(entry->curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(entry->curl_handle, CURLOPT_WRITEDATA, entry);
    curl_easy_setopt(entry->curl_handle, CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(entry->curl_handle, CURLOPT_XFERINFODATA, entry);
    curl_easy_setopt(entry->curl_handle, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(entry->curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(entry->curl_handle, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(entry->curl_handle, CURLOPT_ERRORBUFFER, entry->error_buffer);
    
    if (resume_from > 0) {
        curl_easy_setopt(entry->curl_handle, CURLOPT_RESUME_FROM_LARGE, (curl_off_t)resume_from);
    }
    
    apply_rate_limit(entry->curl_handle, config->rate_limit_kbps);
    
    entry->state = DOWNLOAD_ACTIVE;
    entry->start_time = time(NULL);
    entry->error_buffer[0] = '\0'; // Initialize error buffer
    
    free(full_path);
    return SUCCESS;
}

int start_downloads(Config *config) {
    int active_count = 0;
    
    for (int i = 0; i < config->download_count && active_count < config->max_parallel_downloads; i++) {
        DownloadEntry *entry = &config->downloads[i];
        
        if (entry->state != DOWNLOAD_PENDING) {
            continue;
        }
        
        if (setup_download(entry, config) == SUCCESS) {
            curl_multi_add_handle(multi_handle, entry->curl_handle);
            active_count++;
            log_info("Started download: %s", entry->url);
        } else {
            entry->state = DOWNLOAD_FAILED;
            log_error("Failed to setup download: %s", entry->url);
        }
    }
    
    return active_count;
}

void pause_downloads(Config *config) {
    for (int i = 0; i < config->download_count; i++) {
        DownloadEntry *entry = &config->downloads[i];
        if (entry->state == DOWNLOAD_ACTIVE) {
            entry->state = DOWNLOAD_PAUSED;
            if (entry->curl_handle) {
                curl_multi_remove_handle(multi_handle, entry->curl_handle);
            }
        }
    }
    config->paused = true;
    log_info("All downloads paused");
}

void resume_downloads(Config *config) {
    for (int i = 0; i < config->download_count; i++) {
        DownloadEntry *entry = &config->downloads[i];
        if (entry->state == DOWNLOAD_PAUSED) {
            entry->state = DOWNLOAD_ACTIVE;
            if (entry->curl_handle) {
                curl_multi_add_handle(multi_handle, entry->curl_handle);
            }
        }
    }
    config->paused = false;
    log_info("Downloads resumed");
}

int process_downloads(Config *config) {
    int still_running = 0;
    int active_downloads = start_downloads(config);
    
    if (active_downloads == 0) {
        log_error("No downloads to process");
        return ERROR_DOWNLOAD;
    }
    
    init_progress_display();
    
    while (1) {
        if (should_shutdown()) {
            log_info("Shutdown requested, stopping downloads");
            break;
        }
        
        if (should_toggle_pause()) {
            if (config->paused) {
                resume_downloads(config);
            } else {
                pause_downloads(config);
            }
            reset_pause_toggle();
        }
        
        if (!config->paused) {
            CURLMcode mc = curl_multi_perform(multi_handle, &still_running);
            
            if (mc != CURLM_OK) {
                log_error("curl_multi_perform error: %s", curl_multi_strerror(mc));
                break;
            }
            
            // Update download speeds
            for (int i = 0; i < config->download_count; i++) {
                DownloadEntry *entry = &config->downloads[i];
                if (entry->state == DOWNLOAD_ACTIVE && entry->curl_handle) {
                    curl_off_t speed;
                    curl_easy_getinfo(entry->curl_handle, CURLINFO_SPEED_DOWNLOAD_T, &speed);
                    entry->download_speed = (double)speed;
                }
            }
            
            update_progress_display(config);
            
            // Check for completed downloads
            int msgs_in_queue;
            CURLMsg *msg;
            while ((msg = curl_multi_info_read(multi_handle, &msgs_in_queue))) {
                if (msg->msg == CURLMSG_DONE) {
                    CURL *easy_handle = msg->easy_handle;
                    
                    for (int i = 0; i < config->download_count; i++) {
                        DownloadEntry *entry = &config->downloads[i];
                        if (entry->curl_handle == easy_handle) {
                            curl_multi_remove_handle(multi_handle, easy_handle);
                            curl_easy_cleanup(easy_handle);
                            entry->curl_handle = NULL;
                            
                            if (entry->file) {
                                fclose(entry->file);
                                entry->file = NULL;
                            }
                            
                            if (msg->data.result == CURLE_OK) {
                                entry->state = DOWNLOAD_VERIFYING;
                                log_info("Download completed: %s", entry->url);
                                
                                // Dynamically allocate full path for checksum
                                size_t path_len = snprintf(NULL, 0, "%s/%s", config->download_dir, entry->output_path) + 1;
                                char *full_path = malloc(path_len);
                                if (!full_path) {
                                    log_error("Failed to allocate memory for full path");
                                    entry->state = DOWNLOAD_FAILED;
                                    continue;
                                }
                                snprintf(full_path, path_len, "%s/%s", config->download_dir, entry->output_path);
                                
                                if (config->verify_checksums && strlen(entry->sha256) > 0) {
                                    if (verify_sha256(full_path, entry->sha256)) {
                                        entry->state = DOWNLOAD_COMPLETED;
                                        log_info("Checksum verified: %s", entry->output_path);
                                    } else {
                                        entry->state = DOWNLOAD_FAILED;
                                        log_error("Checksum verification failed: %s", entry->output_path);
                                    }
                                } else {
                                    entry->state = DOWNLOAD_COMPLETED;
                                }
                                
                                free(full_path);
                                
                                // Start next pending download
                                for (int j = 0; j < config->download_count; j++) {
                                    DownloadEntry *next = &config->downloads[j];
                                    if (next->state == DOWNLOAD_PENDING) {
                                        if (setup_download(next, config) == SUCCESS) {
                                            curl_multi_add_handle(multi_handle, next->curl_handle);
                                            log_info("Started next download: %s", next->url);
                                        }
                                        break;
                                    }
                                }
                            } else {
                                entry->state = DOWNLOAD_FAILED;
                                log_error("Download failed: %s - %s (Code: %d) Details: %s", 
                                         entry->url, curl_easy_strerror(msg->data.result), 
                                         msg->data.result, entry->error_buffer);
                            }
                            break;
                        }
                    }
                }
            }
            
            // Check if all downloads are complete
            bool all_done = true;
            for (int i = 0; i < config->download_count; i++) {
                if (config->downloads[i].state != DOWNLOAD_COMPLETED && 
                    config->downloads[i].state != DOWNLOAD_FAILED) {
                    all_done = false;
                    break;
                }
            }
            
            if (all_done) {
                log_info("All downloads completed");
                break;
            }
        }
        
        usleep(100000);
    }
    
    clear_progress_display();
    printf("\n=== Download Summary ===\n");
    int completed = 0, failed = 0;
    for (int i = 0; i < config->download_count; i++) {
        DownloadEntry *entry = &config->downloads[i];
        const char *status = (entry->state == DOWNLOAD_COMPLETED) ? "SUCCESS" : "FAILED";
        printf("[%s] %s\n", status, entry->output_path);
        
        if (entry->state == DOWNLOAD_COMPLETED) completed++;
        else if (entry->state == DOWNLOAD_FAILED) failed++;
    }
    printf("\nCompleted: %d, Failed: %d, Total: %d\n", 
           completed, failed, config->download_count);
    
    return SUCCESS;
}
