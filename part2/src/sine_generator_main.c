#include "base.c"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sine_generator_main.h"

const u64 SIGN_MASK = ((u64)1 << 63);

// generates random f64 value in range of [1;2)
// by simply generating random u64 value and taking the last 52 bits for
// mantissa
// for exponent, all except highest bits are set to true, which means
// value is 1.0f if mantissa is all zero,
// (2 is exclusive because it requires
// a different exponent and the mantissa to be set to 0)
// to get a number in range of [-a;a), simply calculage -3a+<rand_val>+2a
// to get a number in range of [0;a),  simply calculate  -a+<rand_val>+a
f64 rand_f64(u64 *state) {
  typedef union {
    f64 value;
    u64 bits;
  } float_bit_union;
  float_bit_union float_value;
  *state ^= *state << 13;
  *state ^= *state >> 17;
  *state ^= *state << 5;

  // should only set exponent last 10 of 11 bits, so total float should be
  // between 1 and 2
  u64 exponent = ((float_bit_union){.value = 1.0f}).bits;
  u64 mantissa = (*state & 0x000FFFFFFFFFFFFF);
  float_value.bits = exponent | mantissa;
  return float_value.value;
}

f64 rand_f64_in_range(u64 *state, f64 min, f64 max) {
  f64 val = 2 * min - max + rand_f64(state) * (max - min);
  assert(val >= min);
  assert(val < max);
  return val;
}

// could be smaller?
#define MAX_FLOAT_STRING_SIZE 200
#define PARSE_FILE_PATH "sine_data.json"
#define ANSWERS_FILE_PATH "haversine_answers"

HaversineDataSlice gen_formula(bool is_cluster, u64 seed, u64 size) {
  u64 rand_state = seed;
  HaversineDataSlice haversine_data_slice = {
      .ptr = malloc(size * sizeof(HaversineData)),
      .len = size,
  };
  // +20 to account for start and end, could be smaller
  u32 max_file_size = size * MAX_FLOAT_STRING_SIZE + 20;
  u32 parsed_data_i = 0;
  char *parsed_data = malloc(max_file_size);
  parsed_data_i += sprintf(parsed_data + parsed_data_i, "{\"pairs\":[\n");

  f64 x_min_array[16];
  f64 x_max_array[16];
  f64 y_min_array[16];
  f64 y_max_array[16];
  if (is_cluster) {
    for (int i = 0; i < 16; i++) {
      // between -180 and 180
      f64 x_center = -3 * 180 + rand_f64(&rand_state) * 2 * 180;
      f64 y_center = -3 * 90 + rand_f64(&rand_state) * 2 * 90;

      f64 x_max_center = 180 - fabs(x_center);
      f64 y_max_center = 90 - fabs(y_center);
      // between 0 and max_center
      f64 x_radius = -x_max_center + rand_f64(&rand_state) * x_max_center;
      f64 y_radius = -y_max_center + rand_f64(&rand_state) * y_max_center;

      x_min_array[i] = x_center - x_radius;
      y_min_array[i] = y_center - y_radius;

      x_max_array[i] = x_center + x_radius;
      y_max_array[i] = y_center + y_radius;
    }
  } else {
    for (int i = 0; i < 16; i++) {
      x_min_array[i] = -180;
      y_min_array[i] = -90;

      x_max_array[i] = 180;
      y_max_array[i] = 90;
    }
  }

  u32 i = 0;
  u32 cluster_i = 0;

  while (i < size) {
    f64 rand_float = rand_f64_in_range(&rand_state, x_min_array[cluster_i],
                                       x_max_array[cluster_i]);
    haversine_data_slice.ptr[i].x0 = rand_float;
    parsed_data_i +=
        sprintf(parsed_data + parsed_data_i, "    {\"x0\":%.20f", rand_float);

    rand_float = rand_f64_in_range(&rand_state, y_min_array[cluster_i],
                                   y_max_array[cluster_i]);
    haversine_data_slice.ptr[i].y0 = rand_float;
    parsed_data_i +=
        sprintf(parsed_data + parsed_data_i, ", \"y0\":%.20f", rand_float);

    rand_float = rand_f64_in_range(&rand_state, x_min_array[cluster_i],
                                   x_max_array[cluster_i]);
    haversine_data_slice.ptr[i].x1 = rand_float;
    parsed_data_i +=
        sprintf(parsed_data + parsed_data_i, ", \"x1\":%.20f", rand_float);

    rand_float = rand_f64_in_range(&rand_state, y_min_array[cluster_i],
                                   y_max_array[cluster_i]);
    haversine_data_slice.ptr[i].y1 = rand_float;
    parsed_data_i +=
        sprintf(parsed_data + parsed_data_i, ", \"y1\":%.20f},\n", rand_float);
    i += 1;
    cluster_i = (cluster_i + 1) % 16;
  }

  // to remove the , of last element
  parsed_data_i -= 2;
  parsed_data_i += sprintf(parsed_data + parsed_data_i, "\n]}");

  FILE *parse_file = fopen(PARSE_FILE_PATH, "w");
  fwrite(parsed_data, 1, parsed_data_i, parse_file);

  return haversine_data_slice;
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
  F64Slice haversine_answers = {
      .ptr = malloc(haversine_data_slice.len * sizeof(f64)),
      .len = haversine_data_slice.len,
  };
  f64 total = 0;
  for (int i = 0; i < haversine_data_slice.len; i++) {
    HaversineData haversine_data = haversine_data_slice.ptr[i];
    f64 val = Haversine_ReferenceHaversine(haversine_data.x0, haversine_data.y0,
                                           haversine_data.x1, haversine_data.y1,
                                           6372.8);
    haversine_answers.ptr[i] = val;
    total += val;
  }
  f64 sum = total / haversine_data_slice.len;

  FILE *answers_file = fopen(ANSWERS_FILE_PATH, "w");
  fwrite(&sum, sizeof(total), 1, answers_file);
  fwrite(haversine_answers.ptr,
         sizeof(*haversine_answers.ptr) * haversine_answers.len, 1,
         answers_file);

  if (is_cluster) {
    printf("Method: cluster\n");
  } else {
    printf("Method: uniform\n");
  }
  printf("Random seed: %ld\n", seed);
  printf("Pair count: %ld\n", size);
  printf("Expected sum: %f\n", sum);
}
