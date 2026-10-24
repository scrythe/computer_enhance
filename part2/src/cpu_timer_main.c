#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <time.h>
#include <x86intrin.h>

#define IS_MAIN
#include "base.c"

#define NANOSEC_PER_SEC 1e9
#define NANOSEC_PER_MS 1e6

int main(int argc, char *argv[]) {
  u64 ms_wait_time = 1000;

  if (argc > 1) {
    ms_wait_time = parse_u64(argv[1]);
  }

  u64 ns_wait_time = ms_wait_time * NANOSEC_PER_MS;
  u64 os_start = get_nano_sec();
  u64 cpu_start = __rdtsc();
  u64 os_end;
  u64 os_elapsed = 0;
  while (os_elapsed < ns_wait_time) {
    os_end = get_nano_sec();
    os_elapsed = os_end - os_start;
  }
  u64 cpu_end = __rdtsc();
  u64 cpu_elapsed = cpu_end - cpu_start;
  // there are cpu_elapsed (amount) "ticks" in os_elapsed time (nano sec, or
  //  1/NANOSEC_PER_SEC sec), so cpu_elapsed/(os_elpased/NANOSEC_PER_SEC)
  u64 cpu_freq = cpu_elapsed * NANOSEC_PER_SEC / os_elapsed;

  printf("OS 'Freq' / Nanosec per sec: %lu\n", (u64)NANOSEC_PER_MS);
  printf("OS Timer: %lu -> %lu = %lu elapsed\n", os_start, os_end, os_elapsed);
  printf("OS Seconds: %.4f\n", os_elapsed / NANOSEC_PER_SEC);
  printf("CPU Timer: %lu -> %lu = %lu elapsed\n", cpu_start, cpu_end,
         cpu_elapsed);
  printf("CPU Freq: %lu\n", cpu_freq);
  return 0;
}
