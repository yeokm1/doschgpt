void io_timestamp();
void io_app_error(char * str, int length);
void io_server_error(char * str, int length);
void io_str_newline(char * str);
void io_write_str_no_print(char * str, int length);
void io_char(char c);
void io_request_info(unsigned int port, int promptTokens, int completionTokens);

bool io_open_history_file(char * filePath);
void io_close_history_file();