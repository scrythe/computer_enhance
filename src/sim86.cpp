#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../computer_enhance/perfaware/sim86/shared/sim86_shared.h"
#include "sim86.h"

#define SIM86_VERSION 4

#define FILE_NAME "listing_0039_more_movs"
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
  Parse_File_Result parse_file_result =
      parse_file(output_data, (u8 *)input_data, input_file_size);
  int output_data_size = parse_file_result.len;
  int exit_code = parse_file_result.exit_code;
  if (exit_code) {
    return exit_code;
  }
  printf("%s", output_data);
  int exec_err_val =
      execute_and_compare_nasm(output_data, output_data_size, testing_data);
  if (exec_err_val != 0) {
    return exec_err_val;
  }

  return 0;
}

Parse_File_Result parse_file(char *buf, u8 *input_data, int input_file_size) {
  int exit_code = 0;
  int len = 0;
  char temp_buf[1024];
  int temp_len = 0;

  len += sprintf(buf, "; %s disassembly:\nbits 16\n", FILE_NAME);

  int offset = 0;
  while (offset < input_file_size) {
    instruction decoded;
    Sim86_Decode8086Instruction(input_file_size - offset, input_data + offset,
                                &decoded);

    const char *mnemonic = Sim86_MnemonicFromOperationType(decoded.Op);

    const char *args[2];
    for (int i = 0; i < 2; i++) {
      switch (decoded.Operands[i].Type) {
      case Operand_Register:
        args[i] = Sim86_RegisterNameFromOperand(&decoded.Operands[i].Register);
        break;

      case Operand_Immediate: {
        args[i] = temp_buf + temp_len;
        int val = decoded.Operands[i].Immediate.Value;
        temp_len += sprintf(temp_buf + temp_len, "%d", val);
        temp_buf[temp_len] = '\0';
        temp_len += 1;
        break;
      }

      case Operand_Memory: {
        effective_address_expression effec_addr = decoded.Operands[i].Address;
        const char *term_reg_1 =
            Sim86_RegisterNameFromOperand(&effec_addr.Terms[0].Register);
        const char *term_reg_2 =
            Sim86_RegisterNameFromOperand(&effec_addr.Terms[1].Register);
        int displacement = effec_addr.Displacement;

        args[i] = temp_buf + temp_len;
        temp_len += sprintf(temp_buf + temp_len, "[%s", term_reg_1);
        if (strlen(term_reg_2)) {
          temp_len += sprintf(temp_buf + temp_len, " + %s", term_reg_2);
        }
        if (displacement > 0) {
          temp_len += sprintf(temp_buf + temp_len, " + %d", displacement);
        }

        temp_len += sprintf(temp_buf + temp_len, "]");

        break;
      }

      default: {
        exit_code = 1;
      }
      }
    }

    len += sprintf(buf + len, "%s %s, %s\n", mnemonic, args[0], args[1]);
    offset += decoded.Size;
  }
  return Parse_File_Result{.len = len, .exit_code = exit_code};
}

int execute_and_compare_nasm(char *output_data, int output_data_size,
                             char *testing_data) {
  int exit_code = 0;
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

  int status = pclose(nasm_output_file);

  if (WIFEXITED(status)) {
    exit_code = WEXITSTATUS(status);
  }

  nasm_output_buffer[nasm_output_size] = '\0';

  if (exit_code == 0 and strcmp(testing_data, nasm_output_buffer) != 0) {
    printf("Expected:%s\n\nGot:%s\n\n", testing_data, nasm_output_buffer);
    exit_code = 1;
  }
  return exit_code;
}
