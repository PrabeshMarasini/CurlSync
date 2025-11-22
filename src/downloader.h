#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include "../include/curlsync.h"

int init_downloader(void);
void cleanup_downloader(void);
int start_downloads(Config *config);
int process_downloads(Config *config);
void pause_downloads(Config *config);
void resume_downloads(Config *config);

#endif