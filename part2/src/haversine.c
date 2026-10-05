#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef BUILD_DEBUG
#define assert(val)                                                            \
  if (!(val)) {                                                                \
    __builtin_trap();                                                          \
  }
#else
#define assert(val) (void)(val)
#endif

#include "sine_generator.c"

#define printf_error(message, ...)                                             \
  fprintf(stderr, "\033[91mError:\033[0m " message, ##__VA_ARGS__)

f64 Haversine_ReferenceHaversine(f64 X0, f64 Y0, f64 X1, f64 Y1,
                                 f64 EarthRadius);

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

int main(int argc, char *argv[]) {
  int error = 0;
  if (argc < 4) {
    error = 1;
  }
  if (error == 0 && (strcmp(argv[1], "uniform") != 0) &&
      (strcmp(argv[1], "cluster") != 0)) {
    error = 1;
  }

  if (error != 0) {
    printf_error("Require method, seed and size argument\n"
                 "Usage: %s [uniform/cluster] [seed] [size]\n",
                 argv[0]);
    return error;
  }

  bool is_cluster = strcmp(argv[1], "cluster") == 0;
  u64 seed = parse_u64(argv[2]);
  u64 size = parse_u64(argv[3]);

  HaversineDataSlice haversine_data_slice = gen_formula(is_cluster, seed, size);
  f64 total = 0;
  for (int i = 0; i < haversine_data_slice.len; i++) {
    HaversineData haversine_data = haversine_data_slice.ptr[i];
    f64 val = Haversine_ReferenceHaversine(haversine_data.x0, haversine_data.y0,
                                           haversine_data.x1, haversine_data.y1,
                                           6372.8);
    total += val;
  }
  if (is_cluster) {
    printf("Method: cluster\n");
  } else {
    printf("Method: uniform\n");
  }
  printf("Random seed: %ld\n", seed);
  printf("Pair count: %ld\n", size);
  printf("Expected sum: %f\n", total / haversine_data_slice.len);
}
