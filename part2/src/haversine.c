#include "sine_generator.c"

#define printf_error(message, ...)                                             \
  fprintf(stderr, "\033[91mError:\033[0m " message, ##__VA_ARGS__)

f64 Haversine_ReferenceHaversine(f64 X0, f64 Y0, f64 X1, f64 Y1,
                                 f64 EarthRadius);

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

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf_error("Require seed and size argument\n");
    return 1;
  }

  u32 seed = parse_u32(argv[1]);
  u32 size = parse_u32(argv[2]);

  HaversineDataSlice haversine_data_slice = gen_formula(seed, size);
  f64 total = 0;
  for (int i = 0; i < haversine_data_slice.len; i++) {
    HaversineData haversine_data = haversine_data_slice.ptr[i];
    f64 val = Haversine_ReferenceHaversine(haversine_data.x0, haversine_data.y0,
                                           haversine_data.x1, haversine_data.y1,
                                           6372.8);
    total += val;
  }
  printf("%f\n", total / haversine_data_slice.len);
}
