typedef char unsigned u8;

struct Parse_File_Result {
  int len;
  int exit_code;
};

Parse_File_Result parse_file(char *buf, u8 *input_data, int input_file_size);
int execute_and_compare_nasm(char *output_data, int output_data_size,
                             char *testing_data);
