#include <cstdio>
#include <cstdarg>

#include "log.h"

extern FILE *progress_file;
extern FILE *mempool_stats_file;
extern FILE *data_stats_file;
extern FILE *metadata_file;

void log_progress(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    vfprintf(progress_file, fmt, args);
    va_end(args);

    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
}

void log_mempool(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    vfprintf(mempool_stats_file, fmt, args);
    va_end(args);
}

void log_data_stats(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    vfprintf(data_stats_file, fmt, args);
    va_end(args);
}

void log_metadata(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    vfprintf(metadata_file, fmt, args);
    va_end(args);
}
