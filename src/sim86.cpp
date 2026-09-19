#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../computer_enhance/perfaware/sim86/shared/sim86_shared.h"
#include "sim86.h"

#define SIM86_VERSION 4

#define FILE_NAME "listing_0038_many_register_mov"
#define FILE_INPUT_PATH "../computer_enhance/perfaware/part1/" FILE_NAME
#define FILE_DISASSEMBLY_OUTPUT_PATH                                           \
  "../testing_results/" FILE_NAME "_disassembly.asm"
#define FILE_TEST_EXECUTION_PATH                                               \
  "..computer_enhance/perfaware/part1/" FILE_NAME ".txt"

int main(int argc, char *argv[]) {
  u32 version = Sim86_GetVersion();
  if (version != SIM86_VERSION) {
    printf("Incorrect version, expected %d, got %d\n", SIM86_VERSION, version);
  }

  bool execute = false;
  if (argc > 1) {
    if (strcmp(argv[1], "--execute") == 0) {
      execute = true;
    }
  }

  FILE *input_file = fopen(FILE_INPUT_PATH, "rb");
  if (input_file == NULL) {
    printf("unable to open file: %s\n", FILE_INPUT_PATH);
    return 1;
  }
  fseek(input_file, 0, SEEK_END);
  int input_file_size = ftell(input_file);
  rewind(input_file);
  char *input_data = (char *)malloc(input_file_size + 1);
  fread(input_data, 1, input_file_size, input_file);
  fclose(input_file);

  char *testing_data;
  int testing_file_size;
  if (execute) {
    FILE *testing_file = fopen(FILE_TEST_EXECUTION_PATH, "r");
    if (testing_file == NULL) {
      printf("unable to open file: %s\n", FILE_INPUT_PATH);
      return 1;
    }
    fseek(testing_file, 0, SEEK_END);
    testing_file_size = ftell(testing_file);
    rewind(testing_file);
    testing_data = (char *)malloc(testing_file_size + 1);
    fread(testing_data, 1, testing_file_size, testing_file);
    fclose(testing_file);
  } else {
    testing_data = input_data;
    testing_file_size = input_file_size;
  }

  char output_data[1024];
  int output_data_size =
      parse_file(output_data, (u8 *)input_data, input_file_size);
  printf("%s", output_data);
  int exec_err_val =
      execute_and_compare_nasm(output_data, output_data_size, testing_data);
  if (exec_err_val != 0) {
    return exec_err_val;
  }

  return 0;
}

int parse_file(char *buf, u8 *input_data, int input_file_size) {
  int len = 0;

  len += sprintf(buf, "; %s disassembly:\nbits 16\n", FILE_NAME);

  int offset = 0;
  while (offset < input_file_size) {
    instruction decoded;
    Sim86_Decode8086Instruction(input_file_size - offset, input_data + offset,
                                &decoded);

    const char *mnemonic = Sim86_MnemonicFromOperationType(decoded.Op);
    const char *dest_reg =
        Sim86_RegisterNameFromOperand(&decoded.Operands[0].Register);
    const char *second_reg =
        Sim86_RegisterNameFromOperand(&decoded.Operands[1].Register);
    // len += sprintf(buf + len, "%s %s, %s\n", mnemonic, dest_reg, second_reg);
    len += sprintf(buf + len, "%s %s, %s\n", mnemonic, dest_reg, second_reg);
    offset += decoded.Size;
  }
  return len;
}

int execute_and_compare_nasm(char *output_data, int output_data_size,
                             char *testing_data) {
  mkdir("../testing_results", 0751);
  FILE *output_nasm_file = fopen(FILE_DISASSEMBLY_OUTPUT_PATH, "w");
  fwrite(output_data, 1, output_data_size, output_nasm_file);
  fclose(output_nasm_file);

  char nasm_command[1024];
  sprintf(nasm_command, "nasm %s -o /dev/stdout", FILE_DISASSEMBLY_OUTPUT_PATH);

  FILE *nasm_output_file = popen(nasm_command, "r");
  char nasm_output_buffer[1024];
  int nasm_output_size = fread(
      nasm_output_buffer, 1, sizeof(nasm_output_buffer) - 1, nasm_output_file);
  pclose(nasm_output_file);

  nasm_output_buffer[nasm_output_size] = '\0';

  int error_code = 0;
  if (strcmp(testing_data, nasm_output_buffer) != 0) {
    printf("Expected:%s\n\nGot:%s\n\n", testing_data, nasm_output_buffer);
    error_code = 1;
  }
  return error_code;
}
