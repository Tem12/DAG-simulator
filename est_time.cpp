#include <cstdio>
#include <ctime>

#include "est_time.h"

void print_diff_time(time_t time_diff)
{
    if (time_diff < 60) {
        // seconds
        printf("ETA: %lds\n", time_diff);
    } else if (time_diff >= 60 && time_diff < 3600) {
        // minutes
        printf("ETA: %ldm:%02lds\n", time_diff / 60, time_diff % 60);
    } else if (time_diff >= 3600 && time_diff < 86400) {
        // hours
        printf("ETA: %ldh:%02ldm:%02lds", time_diff / 3600, time_diff % 3600 / 60,
               time_diff % 3600 % 60);
    } else {
        // days
        printf("ETA: %ld days, %ldh:%02ldm:%02lds", time_diff / 86400,
               time_diff % 86400 / 3600, time_diff % 86400 % 3600 / 60,
               time_diff % 86400 % 3600 % 60);
    }
}
