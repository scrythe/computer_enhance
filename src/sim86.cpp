#include <cstdint>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../computer_enhance/perfaware/sim86/shared/sim86_shared.h"
#include "sim86.h"

#define SIM86_VERSION 4

#define FILE_NAME "listing_0055_challenge_rectangle"
#define FILE_INPUT_PATH "computer_enhance/perfaware/part1/" FILE_NAME
#define FILE_DISASSEMBLY_OUTPUT_PATH                                           \
  "testing_results/" FILE_NAME "_disassembly.asm"
#define FILE_IMAGE_OUTPUT_PATH "results/" FILE_NAME "_image.data"
#define FILE_TEST_EXECUTION_PATH                                               \
  "computer_enhance/perfaware/part1/" FILE_NAME ".txt"

#define UINT4_MAX 15

#define CX_REG_INDEX 2

const int MEMORY_SIZE = 2 << 15;

enum Flags {
  C,
  P,
  A,
  S,
  O,
  Z,
};

const u8 FlagsCharMap[6] = {'C', 'P', 'A', 'S', 'O', 'Z'};

int main(int argc, char *argv[]) {
  u32 version = Sim86_GetVersion();
  if (version != SIM86_VERSION) {
    printf("Incorrect version, expected %d, got %d\n", SIM86_VERSION, version);
  }

  bool execute = false;
  bool output_image = false;
  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "--execute") == 0) {
      execute = true;
    } else if (strcmp(argv[i], "--output_image") == 0) {
      output_image = true;
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
  input_data[input_file_size] = '\0';
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
    testing_data[testing_file_size] = '\0';
    fread(testing_data, 1, testing_file_size, testing_file);
    fclose(testing_file);
  } else {
    testing_data = input_data;
    testing_file_size = input_file_size;
  }

  int max_output_size =
      2 * testing_file_size > 200 ? 2 * testing_file_size : 2048;
  char *output_data = (char *)malloc(max_output_size);
  // char output_data[2048];
  Decode_Execute_File_Result decode_execute_file_result = decode_execute_file(
      output_data, (u8 *)input_data, input_file_size, execute, output_image);
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

#if defined(__SANITIZE_ADDRESS__)
  free(input_data);
  free(testing_data);
#endif
  return 0;
}

Decode_Execute_File_Result decode_execute_file(char *buf, u8 *input_data,
                                               int input_file_size,
                                               bool execute,
                                               bool output_image) {
  int exit_code = 0;
  int len = 0;
  char temp_buf[2048];
  int temp_len = 0;

  u8 memory[MEMORY_SIZE] = {};
  u16 registers[14] = {};
  bool flags[sizeof(FlagsCharMap)];

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

    int changed_register_index = -1;
    int register_prev_val = 0;
    bool flags_prev_val[sizeof(FlagsCharMap)];
    int ip_prev_val = offset;
    // can change later with jump instruction
    int ip_new_val = offset + decoded.Size;
    memcpy(flags_prev_val, flags, sizeof(flags));

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

        int reg1_val = 0;
        int reg2_val = 0;
        if (effec_addr.Terms[0].Register.Index != 0) {
          reg1_val = registers[effec_addr.Terms[0].Register.Index - 1];
        }
        if (effec_addr.Terms[1].Register.Index != 0) {
          reg2_val = registers[effec_addr.Terms[1].Register.Index - 1];
        }
        args[i] = reg1_val + reg2_val + displacement;
        if (i == 1) {
          if (decoded.Flags == Inst_Wide) {
            args[i] = ((u16 *)memory)[args[i] / 2];
          } else {
            args[i] = memory[args[i]];
          }
        }

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
    if (decoded.Operands[0].Type == Operand_Memory) {
      // and decoded.Operands[1].Type == Operand_Immediate) {
      if (decoded.Flags == Inst_Wide) {
        size = (char *)"word ";
      } else {
        size = (char *)"byte ";
      }
    }

    int16_t res = 0;
    switch (decoded.Op) {
    case Op_mov: {
      res = args[1];
      break;
    }
    case Op_add: {
      bool overflow =
          __builtin_add_overflow((int16_t)args[0], (int16_t)args[1], &res);
      uint16_t unsigned_res;
      bool unsigned_overflow = __builtin_add_overflow(
          (uint16_t)args[0], (uint16_t)args[1], &unsigned_res);
      bool u4_overflow = ((args[0] & 0xF) + (args[1] & 0xF)) > UINT4_MAX;
      u8 number_of_lower_bits = __builtin_popcount(res & 0x00FF);

      flags[(Flags)C] = unsigned_overflow;
      flags[(Flags)P] = number_of_lower_bits % 2 == 0;
      flags[(Flags)A] = u4_overflow;
      flags[(Flags)S] = res < 0;
      flags[(Flags)O] = overflow;
      flags[(Flags)Z] = res == 0;
      break;
    }
    case Op_sub:
    case Op_cmp: {
      bool overflow =
          __builtin_sub_overflow((int16_t)args[0], (int16_t)args[1], &res);
      uint16_t unsigned_res;
      bool unsigned_overflow = __builtin_sub_overflow(
          (uint16_t)args[0], (uint16_t)args[1], &unsigned_res);
      bool u4_overflow = (args[0] & 0xF) < (args[1] & 0xF);
      u8 number_of_lower_bits = __builtin_popcount(res & 0x00FF);

      flags[(Flags)C] = unsigned_overflow;
      flags[(Flags)P] = number_of_lower_bits % 2 == 0;
      flags[(Flags)A] = u4_overflow;
      flags[(Flags)S] = res < 0;
      flags[(Flags)O] = overflow;
      flags[(Flags)Z] = res == 0;
    }
    default: {
    }
    }

    switch (decoded.Op) {
    case Op_mov:
    case Op_add:
    case Op_sub: {
      switch (decoded.Operands[0].Type) {
      case Operand_Register: {
        changed_register_index = decoded.Operands[0].Register.Index - 1;
        register_prev_val = registers[changed_register_index];
        if (decoded.Flags == Inst_Wide) {
          registers[changed_register_index] = res;
        } else {
          ((u8 *)registers)[2 * changed_register_index +
                            decoded.Operands[0].Register.Offset] = res;
        }
        break;
      }
      case Operand_Memory: {
        if (decoded.Flags == Inst_Wide) {
          ((u16 *)memory)[args[0] / 2] = res;
        } else {
          memory[args[0]] = res;
        }
        break;
      }
      default: {
      }
      }
      break;
    }

    case Op_je: {
      if (flags[(Flags)Z]) {
        int jump_offset = decoded.Operands[0].Immediate.Value;
        ip_new_val += jump_offset;
      }
      break;
    }
    case Op_jb: {
      if (flags[(Flags)C]) {
        int jump_offset = decoded.Operands[0].Immediate.Value;
        ip_new_val += jump_offset;
      }
      break;
    }
    case Op_jbe: {
      break;
    }
    case Op_jp: {
      if (flags[(Flags)P]) {
        int jump_offset = decoded.Operands[0].Immediate.Value;
        ip_new_val += jump_offset;
      }
      break;
    }
    case Op_jne: {
      if (!flags[(Flags)Z]) {
        int jump_offset = decoded.Operands[0].Immediate.Value;
        ip_new_val += jump_offset;
      }
      break;
    }

    case Op_loop: {
      changed_register_index = CX_REG_INDEX;
      register_prev_val = registers[changed_register_index];
      register_prev_val = registers[CX_REG_INDEX];
      registers[CX_REG_INDEX] -= 1;
      if (registers[CX_REG_INDEX] != 0) {
        int jump_offset = decoded.Operands[0].Immediate.Value;
        ip_new_val += jump_offset;
      }
      break;
    }
    case Op_loopnz: {
      changed_register_index = CX_REG_INDEX;
      register_prev_val = registers[changed_register_index];
      register_prev_val = registers[CX_REG_INDEX];
      registers[CX_REG_INDEX] -= 1;
      if (registers[CX_REG_INDEX] != 0 and !flags[(Flags)Z]) {
        int jump_offset = decoded.Operands[0].Immediate.Value;
        ip_new_val += jump_offset;
      }
      break;
    }

    default: {
    }
    }

    if (!execute) {
      ip_new_val = offset + decoded.Size;
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

    char *register_change_text = (char *)"";
    bool try_compare_reg = execute and changed_register_index >= 0;
    int register_new_val = 0;
    if (execute and changed_register_index >= 0) {
      register_new_val = registers[changed_register_index];
    }
    // both should be zero if no reg
    if (register_prev_val != register_new_val) {
      register_access reg_word =
          register_access{.Index = (u32)changed_register_index + 1, .Count = 2};
      const char *reg_word_name = Sim86_RegisterNameFromOperand(&reg_word);
      register_change_text = temp_buf + temp_len;
      temp_len += sprintf(temp_buf + temp_len, " %s:0x%x->0x%x", reg_word_name,
                          register_prev_val, register_new_val);
      temp_buf[temp_len] = '\0';
      temp_len += 1;
    }

    char *flags_change_text = (char *)"";
    if (memcmp(flags_prev_val, flags, sizeof(flags)) != 0) {
      flags_change_text = temp_buf + temp_len;
      temp_len += sprintf(temp_buf + temp_len, " flags:");
      for (int i = 0; i < sizeof(flags); i++) {
        if (flags_prev_val[i]) {
          temp_buf[temp_len] = FlagsCharMap[i];
          temp_len += 1;
        }
      }
      temp_len += sprintf(temp_buf + temp_len, "->");
      for (int i = 0; i < sizeof(flags); i++) {
        if (flags[i]) {
          temp_buf[temp_len] = FlagsCharMap[i];
          temp_len += 1;
        }
      }
      temp_buf[temp_len] = '\0';
      temp_len += 1;
    }

    char *ip_change_text = (char *)"";
    if (ip_prev_val != ip_new_val) {
      ip_change_text = temp_buf + temp_len;
      temp_len += sprintf(temp_buf + temp_len, " ip:0x%x->0x%x", ip_prev_val,
                          ip_new_val);
      temp_buf[temp_len] = '\0';
      temp_len += 1;
    }

    len += sprintf(buf + len, "%s %s ;%s%s%s \r\n", mnemonic,
                   instruction_args_text, register_change_text, ip_change_text,
                   flags_change_text);
    temp_len = 0;
    offset = ip_new_val;
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
    if (offset != 0) {
      len += sprintf(buf + len, "      ip: 0x%04x (%d)\r\n", offset, offset);
    }
    bool zero_flags[sizeof(FlagsCharMap)];
    if (memcmp(flags, zero_flags, sizeof(flags)) != 0) {
      len += sprintf(buf + len, "   flags: ");
      for (int i = 0; i < sizeof(flags); i++) {
        if (flags[i]) {
          buf[len] = FlagsCharMap[i];
          len += 1;
        }
      }
      len += sprintf(buf + len, "\r\n");
    }
    len += sprintf(buf + len, "\r\n");
  }
  if (output_image) {
    mkdir("results", 0751);
    FILE *output_image_file = fopen(FILE_IMAGE_OUTPUT_PATH, "w");
    fwrite(memory, 1, MEMORY_SIZE, output_image_file);
    fclose(output_image_file);
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

    int bla = strlen(testing_data);
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

  if (exit_code == 0 and
      (testing_file_size != nasm_output_size or
       memcmp(testing_data, nasm_output_buffer, testing_file_size) != 0)) {
    printf("Expected:\n");
    for (int i = 0; i < testing_file_size; i++) {
      u8 byte = testing_data[i];
      for (int bit_i = 7; bit_i >= 0; bit_i--) {
        int bit = (byte >> bit_i) & 1;
        printf("%d", bit);
      }
      printf(" ");
    }
    printf("\nReceived:\n");
    for (int i = 0; i < nasm_output_size; i++) {
      u8 byte = nasm_output_buffer[i];
      for (int bit_i = 7; bit_i >= 0; bit_i--) {
        int bit = (byte >> bit_i) & 1;
        printf("%d", bit);
      }
      printf(" ");
    }
    exit_code = 1;
  }
  return exit_code;
}
