#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/statvfs.h>
#include <time.h>
#include <unistd.h>

typedef size_t usize;
typedef char u8;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef float f32;
typedef double f64;

#define UNPACK(err, fmt, ...)                                                  \
  if (err != EXIT_SUCCESS) {                                                   \
    fprintf(stderr, fmt __VA_OPT__(, ) __VA_ARGS__);                           \
    exit(EXIT_FAILURE);                                                        \
  }

#define bar_pooltime ((u32)50)

#define bar_cap ((usize)2048)
static u8 bar_buffer[bar_cap];
static usize bar_len;

i32 main(void) {
  // Init //
  i32 err;

  do {
    bar_len = 0;
    bar_len += sprintf(bar_buffer, "/");

    // Disk usage //
    struct statvfs fs_stats;
    u64 fs_used, fs_total;
    f64 fs_usedf, fs_totalf;
    err = statvfs("/", &fs_stats);
    UNPACK(err, "error: could not get / stats [%d]\n", err);
    fs_used = fs_stats.f_bsize * (fs_stats.f_blocks - fs_stats.f_bfree);
    fs_total = fs_stats.f_bsize * fs_stats.f_blocks;
    fs_usedf = (f64)fs_used / 1024.f / 1024.f / 1024.f;
    fs_totalf = (f64)fs_total / 1024.f / 1024.f / 1024.f;
    if (fs_totalf >= 1024.f) {
      bar_len += sprintf(bar_buffer + bar_len, " Disk: %.1f of %.1f TiB /",
                         fs_usedf / 1024.f, fs_totalf / 1024.f);
    } else {
      bar_len += sprintf(bar_buffer + bar_len, " Disk: %.1f of %.1f GiB /",
                         fs_usedf, fs_totalf);
    }

    // RAM usage //
    FILE *fd;
    u64 mem_free, mem_total;
    f64 mem_usedf, mem_totalf;
    fd = fopen("/proc/meminfo", "r");
    err = !fscanf(fd, "MemTotal: %lu kB ", &mem_total);
    UNPACK(err, "error: could not read info from /proc/meminfo [%d]\n", err);
    err = !fscanf(fd, "MemFree: %lu kB ", &mem_free);
    UNPACK(err, "error: could not read info from /proc/meminfo [%d]\n", err);
    err = !fscanf(fd, "MemAvailable: %lu kB ", &mem_free);
    UNPACK(err, "error: could not read info from /proc/meminfo [%d]\n", err);
    mem_usedf = (f64)(mem_total - mem_free) / 1024.f / 1024.f;
    mem_totalf = (f64)mem_total / 1024.f / 1024.f;
    bar_len += sprintf(bar_buffer + bar_len, " RAM: %.1f of %.1f GiB /",
                       mem_usedf, mem_totalf);

    // Date & time //
    time_t now = (time_t)0;
    time(&now);
    bar_len += strftime(bar_buffer + bar_len, bar_cap - bar_len,
                        " %A %d, %B %Y %H:%M:%S /", localtime(&now));

    // Display
    printf("%s\n", bar_buffer);
    fflush(stdout);
  } while (!usleep(bar_pooltime * 1000));
  
  // Cleanup
  return EXIT_SUCCESS;
}
