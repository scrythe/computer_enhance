#define _POSIX_C_SOURCE 200809L
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define IS_MAIN
#include "base.c"
#include "sine_generator_main.h"

#define MAX_PROFILE_POINTS 50

typedef struct {
  u64 profile_time;
  const char *profile_name;
} Profile_Function_Data;

Profile_Function_Data profile_points_list[MAX_PROFILE_POINTS];
u8 profile_points_len = 0;

void calc_time_diff(Profile_Function_Data *profile_function_data) {
  u64 profile_elapsed = __rdtsc() - profile_function_data->profile_time;
  profile_function_data->profile_time = profile_elapsed;
  profile_points_list[profile_points_len++] = *profile_function_data;
}

void program_end(u64 *program_start) {
  u64 program_elapsed = __rdtsc() - *program_start;
  u64 cpu_freq = get_cpu_freq(1000);

  printf("Total time %.4fms\n",
         (f64)program_elapsed / (f64)cpu_freq * MS_PER_SEC);
  for (u8 i = 0; i < profile_points_len; i++) {
    Profile_Function_Data profile_point = profile_points_list[i];
    printf("  %s: %lu (%.4f%%)\n", profile_point.profile_name,
           profile_point.profile_time,
           (f64)profile_point.profile_time / program_elapsed * 100);
  }
}

#define profile_function                                                       \
  __attribute__((cleanup(                                                      \
      calc_time_diff))) Profile_Function_Data profile_data##__LINE__ = {       \
      .profile_time = __rdtsc(),                                               \
      .profile_name = __func__,                                                \
  }

#define profile_program                                                        \
  __attribute__((cleanup(program_end))) u64 program_start##__LINE__ = __rdtsc()

f64 parse_f64(u8 *input_data, u32 *i) {
  *i += 5; // for e.g. "x0":
  bool pos = true;
  i64 integer = 0;
  if (input_data[*i] == '-') {
    pos = false;
    *i += 1;
  }
  while (input_data[*i] >= '0' && input_data[*i] <= '9') {
    integer = integer * 10 + input_data[*i] - '0';
    *i += 1;
  }
  u64 fraction = 0;
  u16 decimal_point_pos = 0;
  if (input_data[*i] == '.') {
    *i += 1;
    while (input_data[*i] >= '0' && input_data[*i] <= '9') {
      fraction = fraction * 10 + input_data[*i] - '0';
      decimal_point_pos += 1;
      *i += 1;
    }
  }
  f64 pos_val = (f64)integer + (f64)fraction * pow(10, -decimal_point_pos);
  return pos ? pos_val : pos_val * -1;
}

f64 haversine_sum(HaversineDataVector haversine_data_vector) {
  profile_function;
  f64 total = 0;
  for (u32 i = 0; i < haversine_data_vector.len; i++) {
    HaversineData haversine_data = haversine_data_vector.ptr[i];
    f64 val = Haversine_ReferenceHaversine(haversine_data.x0, haversine_data.y0,
                                           haversine_data.x1, haversine_data.y1,
                                           6372.8);
    total += val;
  }

  return total / haversine_data_vector.len;
}

u8 parse_json(HaversineDataVector *haversine_data_vector, u8 *input_data,
              u32 input_file_size) {
  profile_function;
  char *expected_input_start = "{\"pairs\":[\n";
  u32 expected_input_start_size = strlen(expected_input_start);
  // char *expected_input_end = "]}";

  bool parse_error = 0;
  if (input_file_size < expected_input_start_size) {
    parse_error = true;
  }
  if (!parse_error &&
      memcmp(expected_input_start, input_data, expected_input_start_size)) {
    parse_error = true;
  }
  if (parse_error) {
    printf_error("File invalid, must have %s at the beginning\n",
                 expected_input_start);
    return 1;
  }
  u32 i = expected_input_start_size;
  while (i < input_file_size) {
    if (input_data[i] == ' ') {
    } else if (input_data[i] == '{') {
      i += 1;

      for (u8 i2 = 0; i2 < 4; i2++) {
        if (memcmp("\"x0\":", input_data + i, 5) == 0) {
          haversine_data_vector->ptr[haversine_data_vector->len].x0 =
              parse_f64(input_data, &i);
        } else if (memcmp("\"y0\":", input_data + i, 5) == 0) {
          haversine_data_vector->ptr[haversine_data_vector->len].y0 =
              parse_f64(input_data, &i);
        } else if (memcmp("\"x1\":", input_data + i, 5) == 0) {
          haversine_data_vector->ptr[haversine_data_vector->len].x1 =
              parse_f64(input_data, &i);
        } else if (memcmp("\"y1\":", input_data + i, 5) == 0) {
          haversine_data_vector->ptr[haversine_data_vector->len].y1 =
              parse_f64(input_data, &i);
        } else {
          printf_error("Expected \"x0\":, \"y0\": \"x1\": or \"y1\": at pos "
                       "%d, instead "
                       "got %.*s\n",
                       i, 5, input_data + i);
          return 1;
        }
        if (i2 < 3 && memcmp(", ", input_data + i, 2) != 0) {
          printf_error("Expected ', ' after value at pos %d\n", i);
          return 1;
        } else {
          i += 2;
        }
      }
      haversine_data_vector->len += 1;
    }
    i += 1;
  }
  return 0;
}

u8 read_file(char *input_filename, u32 *input_file_size, u8 **input_data) {
  profile_function;
  FILE *input_file = fopen(input_filename, "r");
  if (input_file == NULL) {
    printf_error("Unable to open file '%s'\n", input_filename);
    return 1;
  } else {
    fseek(input_file, 0, SEEK_END);
    *input_file_size = ftell(input_file);
    rewind(input_file);
    *input_data = malloc(*input_file_size);
    fread(*input_data, 1, *input_file_size, input_file);
    return 0;
  }
}

typedef enum {
  a_x0,
  a_y0,
  a_x1,
  a_y1,
} Attribute;

int main(int argc, char *argv[]) {
  profile_program;
  int error = 0;
  if (argc < 2) {
    printf_error("Require json filename and optional answer filename argument\n"
                 "Usage: %s [haversine_input.json]\n"
                 "       %s [haversine_input.json] [answer.f64]\n",
                 argv[0], argv[0]);
    return 1;
  }

  char *input_filename = argv[1];
  u32 input_file_size;
  u8 *input_data;
  error = read_file(input_filename, &input_file_size, &input_data);
  if (error)
    return error;

  u32 max_haversine_amount = input_file_size / 100;
  HaversineDataVector haversine_data_vector = {
      .ptr = malloc(sizeof(HaversineData) * max_haversine_amount),
      .len = 0,
      .capacity = max_haversine_amount,
  };

  // HaversineDataVector haversine_data_vector;
  error = parse_json(&haversine_data_vector, input_data, input_file_size);
  if (error)
    return error;

  f64 sum = haversine_sum(haversine_data_vector);

  printf("Haversine sum: %0.20f\n", sum);

  if (argc > 2) {
    char *answers_filename = argv[2];
    FILE *answers_file = fopen(answers_filename, "r");
    if (answers_file == NULL) {
      printf_error("Unable to open file '%s'\n", answers_filename);
      return 1;
    }
    f64 answer;
    fread(&answer, sizeof(answer), 1, answers_file);
    printf("\nValidation:\n"
           "Reference sum: %0.20f\n"
           "Difference: %0.20f\n",
           answer, answer - sum);
  }
}
