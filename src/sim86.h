typedef char unsigned u8;

struct Decode_Execute_File_Result {
  int len;
  int exit_code;
};

Decode_Execute_File_Result decode_execute_file(char *buf, u8 *input_data,
                                               int input_file_size,
                                               bool execute, bool output_iamge);
int compare_decoded_asm(char *output_data, int output_data_size,
                        char *testing_data, int testing_file_size);
int compare_executed_asm(char *output_data, int output_data_size,
                         char *testing_data, int testing_file_size);
