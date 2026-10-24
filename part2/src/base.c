#ifndef IS_MAIN
#define _POSIX_C_SOURCE 200809L
#include <time.h>
#include <x86intrin.h>
#endif

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

#define NANOSEC_PER_SEC 1e9
#define NANOSEC_PER_MS 1e6
#define MS_PER_SEC 1e3

f64 Haversine_ReferenceHaversine(f64 X0, f64 Y0, f64 X1, f64 Y1,
                                 f64 EarthRadius);

#ifdef BUILD_DEBUG
#define assert(val)                                                            \
  if (!(val)) {                                                                \
    __builtin_trap();                                                          \
  }
#else
#define assert(val) (void)(val)
#endif

#define printf_error(message, ...)                                             \
  fprintf(stderr, "\033[91mError:\033[0m " message, ##__VA_ARGS__)

u64 parse_u64(char *string) {
  u64 value = 0;
  char *current_char = string;
  while (*current_char >= '0' && *current_char <= '9') {
    u8 integer = *current_char - '0';
    value = value * 10 + integer;
    current_char += 1;
  }
  return value;
}

u64 get_nano_sec() {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return ts.tv_sec * NANOSEC_PER_SEC + ts.tv_nsec;
}

u64 get_cpu_freq(u64 ms_wait_time) {
  u64 ns_wait_time = ms_wait_time * NANOSEC_PER_MS;
  u64 os_start = get_nano_sec();
  u64 cpu_start = __rdtsc();
  u64 os_elapsed = 0;
  while (os_elapsed < ns_wait_time) {
    os_elapsed = get_nano_sec() - os_start;
  }
  u64 cpu_elapsed = __rdtsc() - cpu_start;
  // there are cpu_elapsed (amount) "ticks" in os_elapsed time (nano sec, or
  //  1/NANOSEC_PER_SEC sec), so cpu_elapsed/(os_elpased/NANOSEC_PER_SEC)
  return cpu_elapsed * NANOSEC_PER_SEC / os_elapsed;
}
