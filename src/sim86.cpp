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

#define FILE_NAME "listing_0045_challenge_register_movs"
#define FILE_INPUT_PATH "computer_enhance/perfaware/part1/" FILE_NAME
#define FILE_DISASSEMBLY_OUTPUT_PATH                                           \
  "testing_results/" FILE_NAME "_disassembly.asm"
#define FILE_TEST_EXECUTION_PATH                                               \
  "computer_enhance/perfaware/part1/" FILE_NAME ".txt"

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

  char output_data[2048];
  Decode_Execute_File_Result decode_execute_file_result = decode_execute_file(
      output_data, (u8 *)input_data, input_file_size, execute);
  int output_data_size = decode_execute_file_result.len;
  int exit_code = decode_execute_file_result.exit_code;
  if (exit_code) {
    return exit_code;
  }
  int exec_err_val = 0;
  if (!execute) {
    exec_err_val = compare_decoded_asm(output_data, output_data_size,
                                       testing_data, testing_file_size);
  } else {
    exec_err_val = compare_executed_asm(output_data, output_data_size,
                                        testing_data, testing_file_size);
  }
  if (exec_err_val != 0) {
    return exec_err_val;
  }
  printf("%s", output_data);

  return 0;
}

Decode_Execute_File_Result decode_execute_file(char *buf, u8 *input_data,
                                               int input_file_size,
                                               bool execute) {
  int exit_code = 0;
  int len = 0;
  char temp_buf[2048];
  int temp_len = 0;

  u16 registers[14] = {};

  if (execute) {
    len += sprintf(buf, "--- test\\%s execution ---\r\n", FILE_NAME);
  } else {
    len += sprintf(buf, "; %s disassembly:\nbits 16\n", FILE_NAME);
  }

  int offset = 0;
  while (offset < input_file_size) {
    instruction decoded;
    Sim86_Decode8086Instruction(input_file_size - offset, input_data + offset,
                                &decoded);

    int index = decoded.Operands[0].Register.Index - 1;
    int register_prev_val = registers[index];

    const char *mnemonic = Sim86_MnemonicFromOperationType(decoded.Op);

    const char *args_text[2];
    int args[2];
    for (int i = 0; i < 2; i++) {
      switch (decoded.Operands[i].Type) {
      case Operand_Register: {

        args_text[i] =
            Sim86_RegisterNameFromOperand(&decoded.Operands[i].Register);
        int operand_reg_index = decoded.Operands[i].Register.Index - 1;
        int val;
        if (decoded.Operands[i].Register.Count == 2) {
          val = registers[operand_reg_index];
        } else {
          val = ((u8 *)registers)[2 * operand_reg_index +
                                  decoded.Operands[i].Register.Offset];
        }

        args[i] = val;
        break;
      }

      case Operand_Immediate: {
        args_text[i] = temp_buf + temp_len;
        int val = decoded.Operands[i].Immediate.Value;
        args[i] = val;
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

        args_text[i] = temp_buf + temp_len;
        temp_len += sprintf(temp_buf + temp_len, "[%s", term_reg_1);
        if (strlen(term_reg_2)) {
          temp_len += sprintf(temp_buf + temp_len, "+%s", term_reg_2);
        }
        if (displacement != 0) {
          temp_len += sprintf(temp_buf + temp_len, "+%d", displacement);
        }

        temp_len += sprintf(temp_buf + temp_len, "]");

        temp_buf[temp_len] = '\0';
        temp_len += 1;

        break;
      }

      case Operand_None: {
      }
      }
    }

    char *size = (char *)"";
    if (decoded.Operands[0].Type == Operand_Memory and
        decoded.Operands[1].Type == Operand_Immediate) {
      if (decoded.Flags == Inst_Wide) {
        size = (char *)"word ";
      } else {
        size = (char *)"byte ";
      }
    }

    switch (decoded.Op) {
    case Op_mov: {
      if (decoded.Operands[0].Register.Count == 2) {
        registers[index] = args[1];
      } else {
        ((u8 *)registers)[2 * index + decoded.Operands[0].Register.Offset] =
            args[1];
      }
      break;
    }
    default: {
    }
    }

    char *instruction_args_text = temp_buf + temp_len;
    switch (decoded.Op) {
    case Op_mov:
    case Op_add:
    case Op_sub:
    case Op_cmp: {
      temp_len += sprintf(temp_buf + temp_len, "%s%s, %s", size, args_text[0],
                          args_text[1]);
      temp_buf[temp_len] = '\0';
      temp_len += 1;
      break;
    }
    case Op_je:
    case Op_jl:
    case Op_jle:
    case Op_jb:
    case Op_jbe:
    case Op_jp:
    case Op_jo:
    case Op_js:
    case Op_jne:
    case Op_jnl:
    case Op_jg:
    case Op_jnb:
    case Op_ja:
    case Op_jnp:
    case Op_jno:
    case Op_jns:
    case Op_loop:
    case Op_loopz:
    case Op_loopnz:
    case Op_jcxz: {
      int offset = decoded.Operands[0].Immediate.Value + 2;
      char sign = offset > 0 ? '+' : '-';
      temp_len += sprintf(temp_buf + temp_len, "$%c%d", sign, abs(offset));
      temp_buf[temp_len] = '\0';
      temp_len += 1;
      break;
    }
    default: {
    }
    }

    int register_new_val = registers[index];
    char *register_change_text = (char *)"";
    if (execute == true and register_prev_val != register_new_val) {
      register_access reg_word = register_access{
          .Index = decoded.Operands[0].Register.Index, .Count = 2};
      const char *reg_word_name = Sim86_RegisterNameFromOperand(&reg_word);
      register_change_text = temp_buf + temp_len;
      temp_len += sprintf(temp_buf + temp_len, " ; %s:0x%x->0x%x",
                          reg_word_name, register_prev_val, register_new_val);
      temp_buf[temp_len] = '\0';
      temp_len += 1;
    }

    len += sprintf(buf + len, "%s %s%s \r\n", mnemonic, instruction_args_text,
                   register_change_text);
    offset += decoded.Size;
  }

  if (execute) {
    len += sprintf(buf + len, "\r\nFinal registers:\r\n");
    for (int unsigned i = 0; i < sizeof(registers) / 2; i += 1) {
      register_access reg =
          register_access{.Index = i + 1, .Offset = 0, .Count = 2};
      int register_val = registers[i];
      if (register_val != 0) {
        const char *register_name = Sim86_RegisterNameFromOperand(&reg);
        len += sprintf(buf + len, "      %s: 0x%04x (%d)\r\n", register_name,
                       register_val, register_val);
      }
    }
    len += sprintf(buf + len, "\r\n");
  }
  return Decode_Execute_File_Result{.len = len, .exit_code = exit_code};
}

// inspired a bit by zigs testing.expectEqualStrings
int compare_executed_asm(char *output_data, int output_data_size,
                         char *testing_data, int testing_file_size) {
  int exit_code = 0;

  int diff_i = 0;
  int line_count = 0;
  int line_start = 0;
  // conveniently diff_i is also the i at end of loop if size is not equal
  // (otherwise can be used to check no difference)
  for (; diff_i < testing_file_size and diff_i < output_data_size; diff_i++) {
    if (testing_data[diff_i] != output_data[diff_i]) {
      break;
    }
    if (testing_data[diff_i] == '\n') {
      line_start = diff_i + 1;
      line_count += 1;
    }
  }

  int testing_data_line_end = diff_i;
  for (; testing_data_line_end < testing_file_size; testing_data_line_end++) {
    if (testing_data[testing_data_line_end] == '\n')
      break;
  }

  int output_data_line_end = diff_i;
  for (; output_data_line_end < output_data_size; output_data_line_end++) {
    if (output_data[output_data_line_end] == '\n')
      break;
  }

  if (testing_file_size != output_data_size or testing_file_size != diff_i) {
    exit_code = 1;

    printf("expected:\n%s\nreceived:\n%s\n", testing_data, output_data);

    printf("\nfirst difference in line %d:\n", line_count);

    printf("expected:\n");
    printf("%.*s\n", testing_data_line_end - line_start,
           testing_data + line_start);
    for (int i = 0; i < diff_i - line_start; i++) {
      printf(" ");
    }
    printf("^ (0x%02x)\n", *(testing_data + diff_i));

    printf("received:\n");
    printf("%.*s\n", output_data_line_end - line_start,
           output_data + line_start);
    for (int i = 0; i < diff_i - line_start; i++) {
      printf(" ");
    }
    printf("^ (0x%02x)\n", *(output_data + diff_i));
  }

  return exit_code;
}

int compare_decoded_asm(char *output_data, int output_data_size,
                        char *testing_data, int testing_file_size) {
  int exit_code = 0;
  mkdir("testing_results", 0751);
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

  if (exit_code == 0 and testing_file_size != nasm_output_size and
      memcmp(testing_data, nasm_output_buffer, testing_file_size) != 0) {
    printf("Expected:\n%s\n\nGot:\n%s\n\n", testing_data, nasm_output_buffer);
    exit_code = 1;
  }
  return exit_code;
}
