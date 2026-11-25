#define _POSIX_C_SOURCE 200809L
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define IS_MAIN
#include "base.c"
#include "profile.c"

#include "json_parser.c"

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

u8 read_file(char *input_filename, slice(u8) * input_data) {
  // profile_function;
  FILE *input_file = fopen(input_filename, "r");
  if (input_file == NULL) {
    printf_error("Unable to open file '%s'\n", input_filename);
    return 1;
  } else {
    fseek(input_file, 0, SEEK_END);
    u32 input_file_size = ftell(input_file);
    rewind(input_file);
    input_data->ptr = malloc(input_file_size);
    time_bandwith(__func__, input_file_size);
    fread(input_data->ptr, 1, input_file_size, input_file);
    input_data->len = input_file_size;
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
  profile_begin;
  int error = 0;
  if (argc < 2) {
    printf_error("Require json filename and optional answer filename argument\n"
                 "Usage: %s [haversine_input.json]\n"
                 "       %s [haversine_input.json] [answer.f64]\n",
                 argv[0], argv[0]);
    return 1;
  }

  char *input_filename = argv[1];
  // u8 *input_data;
  slice(u8) input_data = {};
  error = read_file(input_filename, &input_data);
  if (error)
    return error;

  // bad way of doing it ig but whatever
  u32 max_haversine_amount = input_data.len / 100;
  HaversineDataVector haversine_data_vector = {
      .ptr = malloc(sizeof(HaversineData) * max_haversine_amount),
      .len = 0,
      .capacity = max_haversine_amount,
  };

  Json_Output json_output = parse_json(input_data);
  parse_haversine_data(json_output, &haversine_data_vector);
  // error = parse_json(&haversine_data_vector, input_data, input_file_size);

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
  profile_end;
}

assert_max_profile_points;
