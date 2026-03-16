#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>
#include <time.h>
#include <unistd.h>

typedef size_t usize;
typedef char u8;
typedef int32_t i32;
typedef uint64_t u64;
typedef double f64;

#define UNPACK(err, fmt, ...)                                                  \
  if ((err) != EXIT_SUCCESS) {                                                 \
    fprintf(stderr, fmt __VA_OPT__(, ) __VA_ARGS__);                           \
    exit(EXIT_FAILURE);                                                        \
  }

#define DEFER(...) for (i32 _Z = 0; _Z < 1; _Z++ __VA_OPT__(, ) __VA_ARGS__)

typedef struct {
  const char *name;
  FILE *fd;
} file_t;

static inline file_t file_open(const char *name) {
  FILE *fd = fopen(name, "r");
  UNPACK(fd == NULL, "error: could not open file %s", name);
  return (file_t){.name = name, .fd = fd};
}

static inline void file_drop(file_t *self) { fclose(self->fd); }

#define bar_pooltime ((u64)250)

#define bar_cap ((usize)2048)
static u8 bar_buffer[bar_cap];
static usize bar_len;

i32 main(void) {
  // Init //
  i32 err;
  u64 rx_bytesu, tx_bytesu;
  do {
    bar_len = 0;
    *bar_buffer = '\0';
    
    // RX & TX //
    file_t rx_bytes, tx_bytes;
    u64 buf[2] = {rx_bytesu, tx_bytesu};
    f64 rx_rate, tx_rate;
    rx_bytes = file_open("/sys/class/net/wlp2s0/statistics/rx_bytes");
    tx_bytes = file_open("/sys/class/net/wlp2s0/statistics/tx_bytes");
    DEFER(file_drop(&rx_bytes), file_drop(&tx_bytes)) {
      err = !fscanf(rx_bytes.fd, "%lu", &rx_bytesu);
      UNPACK(err, "could not get info from rx_bytes [%d]", err);
      err = !fscanf(tx_bytes.fd, "%lu", &tx_bytesu);
      UNPACK(err, "could not get info from tx_bytes [%d]", err);
    }
    rx_rate = (f64)(rx_bytesu - buf[0]) / 1024.f;
    tx_rate = (f64)(tx_bytesu - buf[1]) / 1024.f;
    bar_len += sprintf(bar_buffer + bar_len, " rx: %.1f %s | tx: %.1f %s |",
                       (rx_rate >= 1024.f) ? rx_rate / 1024.f : rx_rate,
                       (rx_rate >= 1024.f) ? "Mbps" : "Kbps",
                       (tx_rate >= 1024.f) ? tx_rate / 1024.f : tx_rate,
                       (tx_rate >= 1024.f) ? "Mbps" : "Kbps");

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
    bar_len += sprintf(bar_buffer + bar_len, " SSD: %.1f of %.1f %s |",
                       (fs_totalf >= 1024.f) ? fs_usedf / 1024.f : fs_usedf,
                       (fs_totalf >= 1024.f) ? fs_totalf / 1024.f : fs_totalf,
                       (fs_totalf >= 1024.f) ? "TiB" : "GiB");

    // RAM usage //
    u64 mem_free, mem_total;
    f64 mem_usedf, mem_totalf;
    file_t meminfo = file_open("/proc/meminfo");
    DEFER(file_drop(&meminfo)) {
      err = !fscanf(meminfo.fd, "MemTotal: %lu kB ", &mem_total);
      UNPACK(err, "error: could not read info from /proc/meminfo [%d]\n", err);
      err = !fscanf(meminfo.fd, "MemFree: %lu kB ", &mem_free);
      UNPACK(err, "error: could not read info from /proc/meminfo [%d]\n", err);
      err = !fscanf(meminfo.fd, "MemAvailable: %lu kB ", &mem_free);
      UNPACK(err, "error: could not read info from /proc/meminfo [%d]\n", err);
    }
    mem_usedf = (f64)(mem_total - mem_free) / 1024.f / 1024.f;
    mem_totalf = (f64)mem_total / 1024.f / 1024.f;
    bar_len += sprintf(bar_buffer + bar_len, " RAM: %.1f of %.1f GiB |",
                       mem_usedf, mem_totalf);

    // Date & time //
    time_t now = (time_t)0;
    time(&now);
    bar_len += strftime(bar_buffer + bar_len, bar_cap - bar_len,
                        " %A %d, %B %Y %H:%M:%S ", localtime(&now));

    // Display //
    printf("%s\n", bar_buffer);
    fflush(stdout);
  } while (!usleep(bar_pooltime * 1000));

  // Cleanup //
  return EXIT_SUCCESS;
}
