#include "rate_limiter.h"
void apply_rate_limit(CURL *curl, int rate_limit_kbps) {
    if (rate_limit_kbps > 0) {
        curl_off_t max_speed = (curl_off_t)rate_limit_kbps * 1024;
        curl_easy_setopt(curl, CURLOPT_MAX_RECV_SPEED_LARGE, max_speed);
    }
}