#ifndef RATE_LIMITER_H
#define RATE_LIMITER_H

#include <curl/curl.h>

void apply_rate_limit(CURL *curl, int rate_limit_kbps);

#endif