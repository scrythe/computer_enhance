#include "sine_generator.h"
#include <stdio.h>
#include <stdlib.h>

const u64 SIGN_MASK = ((u64)1 << 63);

f32 rand_f64(u64 *state) {
  typedef union {
    f64 value;
    u64 bits;
  } float_bit_union;
  float_bit_union float_value;
  *state ^= *state << 13;
  *state ^= *state >> 17;
  *state ^= *state << 5;

  u64 sign = *state & SIGN_MASK;
  // from -10 to 10 (+1023 so from 1013 to 1033, 21 is 10*2 plus 1 for inclusive
  // end end)
  u64 exponent_raw = ((*state >> 52) % 21) + (1023 - 10);
  u64 exponent = exponent_raw << 52;
  u64 mantissa = (*state & 0x000FFFFFFFFFFFFF);
  float_value.bits = sign | exponent | mantissa;
  // printf("%f\n", float_value.value);
  return float_value.value;
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
    f64 rand_float = rand_f64(&rand_state);
    haversine_data_slice.ptr[i].x0 = rand_float;
    parsed_data_i +=
        sprintf(parsed_data + parsed_data_i, "    {\"x0\":%f", rand_float);

    rand_float = rand_f64(&rand_state);
    haversine_data_slice.ptr[i].y0 = rand_float;
    parsed_data_i +=
        sprintf(parsed_data + parsed_data_i, ", \"y0\":%f", rand_float);

    rand_float = rand_f64(&rand_state);
    haversine_data_slice.ptr[i].x1 = rand_float;
    parsed_data_i +=
        sprintf(parsed_data + parsed_data_i, ", \"x1\":%f", rand_float);

    rand_float = rand_f64(&rand_state);
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
