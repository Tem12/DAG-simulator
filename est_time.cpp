#include <cstdio>
#include <ctime>

#include "est_time.h"
#include "log.h"

void print_diff_time(time_t time_diff)
{
    if (time_diff < 60) {
        // seconds
        log_progress("ETA: %lds", time_diff);
    } else if (time_diff >= 60 && time_diff < 3600) {
        // minutes
        log_progress("ETA: %ldm:%02lds", time_diff / 60, time_diff % 60);
    } else if (time_diff >= 3600 && time_diff < 86400) {
        // hours
        log_progress("ETA: %ldh:%02ldm:%02lds", time_diff / 3600, time_diff % 3600 / 60,
               time_diff % 3600 % 60);
    } else {
        // days
        log_progress("ETA: %ld days, %ldh:%02ldm:%02lds", time_diff / 86400,
               time_diff % 86400 / 3600, time_diff % 86400 % 3600 / 60,
               time_diff % 86400 % 3600 % 60);
    }
}
