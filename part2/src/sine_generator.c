#include "sine_generator.h"
#include <stdio.h>
#include <stdlib.h>

const u64 SIGN_MASK = ((u64)1 << 63);

// between 1 and 2
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

f64 rand_y_lattitude(u64 *state) {
  f64 rand_value = rand_f64(state);
  return -270 + rand_value * 180;
}

f64 rand_x_longitude(u64 *state) {
  f64 rand_value = rand_f64(state);
  return -540 + rand_value * 360;
}

// could be smaller?
#define MAX_FLOAT_STRING_SIZE 100
#define PARSE_FILE_PATH "sine_data.json"

HaversineDataSlice gen_formula(u32 seed, u32 size) {
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

  int i = 0;
  while (i < size) {
    f64 rand_float = rand_x_longitude(&rand_state);
    haversine_data_slice.ptr[i].x0 = rand_float;
    parsed_data_i +=
        sprintf(parsed_data + parsed_data_i, "    {\"x0\":%f", rand_float);

    rand_float = rand_y_lattitude(&rand_state);
    haversine_data_slice.ptr[i].y0 = rand_float;
    parsed_data_i +=
        sprintf(parsed_data + parsed_data_i, ", \"y0\":%f", rand_float);

    rand_float = rand_x_longitude(&rand_state);
    haversine_data_slice.ptr[i].x1 = rand_float;
    parsed_data_i +=
        sprintf(parsed_data + parsed_data_i, ", \"x1\":%f", rand_float);

    rand_float = rand_y_lattitude(&rand_state);
    haversine_data_slice.ptr[i].y1 = rand_float;
    parsed_data_i +=
        sprintf(parsed_data + parsed_data_i, ", \"y1\":%f},\n", rand_float);
    i += 1;
  }

  // to remove the , of last element
  parsed_data_i -= 2;
  parsed_data_i += sprintf(parsed_data + parsed_data_i, "\n]}");

  FILE *parse_file = fopen(PARSE_FILE_PATH, "w");
  fwrite(parsed_data, 1, parsed_data_i, parse_file);

  return haversine_data_slice;
}
