#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../computer_enhance/perfaware/sim86/shared/sim86_shared.h"
// #include "sim86_shared.h"
#include "sim86.h"

#define SIM86_VERSION 4

#define FILE_NAME "listing_0057_challenge_cycles"
#define FILE_INPUT_PATH "../computer_enhance/perfaware/part1/" FILE_NAME
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
  long input_file_size = ftell(input_file);
  rewind(input_file);
  char *input_data = (char *)malloc(input_file_size + 1);
  fread(input_data, 1, input_file_size, input_file);
  fclose(input_file);

  char *testing_data;
  if (execute) {
    FILE *testing_file = fopen(FILE_TEST_EXECUTION_PATH, "r");
    if (testing_file == NULL) {
      printf("unable to open file: %s\n", FILE_INPUT_PATH);
      return 1;
    }
    fseek(testing_file, 0, SEEK_END);
    long testing_file_size = ftell(testing_file);
    rewind(testing_file);
    testing_data = (char *)malloc(testing_file_size + 1);
    fread(testing_data, 1, testing_file_size, testing_file);
    fclose(testing_file);
  } else {
    testing_data = input_data;
  }

  char buf[1024];
  int len = parse_file(buf, (u8 *)input_data, input_file_size);
  printf("%s", buf);

  return 0;
}

int parse_file(char *buf, u8 *input_data, int input_file_size) {
  int len = 0;

  int offset = 0;
  while (offset < input_file_size) {
    instruction decoded;
    Sim86_Decode8086Instruction(input_file_size - offset, input_data + offset,
                                &decoded);

    const char *mnemonic = Sim86_MnemonicFromOperationType(decoded.Op);
    const char *dest_reg =
        Sim86_RegisterNameFromOperand(&decoded.Operands[0].Register);
    const char *second_reg =
        Sim86_RegisterNameFromOperand(&decoded.Operands[0].Register);
    len += sprintf(buf + len, "%s %s, %s\n", mnemonic, dest_reg, second_reg);
    offset += decoded.Size;
  }

  return len;
}
