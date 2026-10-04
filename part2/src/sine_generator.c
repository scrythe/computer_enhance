#include "sine_generator.h"
#include <stdio.h>
#include <stdlib.h>

#define printf_error(message, ...)                                             \
  fprintf(stderr, "\033[91mError:\033[0m " message, ##__VA_ARGS__)

f32 rand_f32(u32 *state) {
  typedef union {
    f32 value;
    u32 bits;
  } float_bit_union;
  float_bit_union float_value;
  *state ^= *state << 13;
  *state ^= *state >> 17;
  *state ^= *state << 5;

  u32 sign = *state & 0x80000000;
  // from -15 to 15 (+127 so from 112 to 142, 31 is 15*2 plus 1 for inclusive
  // end)
  u32 exponent_raw = ((*state >> 23) % 31) + 112;
  u32 exponent = exponent_raw << 23;
  u32 mantissa = (*state & 0x007FFFFF);
  float_value.bits = sign | exponent | mantissa;
  return float_value.value;
}

u32 parse_u32(char *string) {
  u32 value = 0;
  char *current_char = string;
  while (*current_char >= '0' && *current_char <= '9') {
    u8 integer = *current_char - '0';
    value = value * 10 + integer;
    current_char += 1;
  }
  return value;
}

// could be smaller?
#define MAX_FLOAT_STRING_SIZE 100
#define PARSE_FILE_PATH "sine_data.json"

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf_error("Require seed and size argument\n");
    return 1;
  }

  u32 seed = parse_u32(argv[1]);
  u32 rand_state = seed;
  u32 size = parse_u32(argv[2]);
  // +20 to account for start and end, could be smaller
  u32 max_file_size = size * MAX_FLOAT_STRING_SIZE + 20;
  u32 parsed_data_i = 0;
  char *parsed_data = malloc(max_file_size);
  parsed_data_i += sprintf(parsed_data + parsed_data_i, "{\"pairs\":[\n");

  int i = 0;
  while (i < size) {
    f32 rand_float = rand_f32(&rand_state);
    parsed_data_i +=
        sprintf(parsed_data + parsed_data_i, "    {\"x0\":%f", rand_float);

    rand_float = rand_f32(&rand_state);
    parsed_data_i +=
        sprintf(parsed_data + parsed_data_i, ", \"y0\":%f", rand_float);

    rand_float = rand_f32(&rand_state);
    parsed_data_i +=
        sprintf(parsed_data + parsed_data_i, ", \"x1\":%f", rand_float);

    rand_float = rand_f32(&rand_state);
    parsed_data_i +=
        sprintf(parsed_data + parsed_data_i, ", \"y1\":%f},\n", rand_float);
    i += 1;
  }

  // to remove the , of last element
  parsed_data_i -= 2;
  parsed_data_i += sprintf(parsed_data + parsed_data_i, "\n]}");

  FILE *parse_file = fopen(PARSE_FILE_PATH, "w");
  u32 written = fwrite(parsed_data, 1, parsed_data_i, parse_file);

  printf("%.*s\n", parsed_data_i, parsed_data);
}
