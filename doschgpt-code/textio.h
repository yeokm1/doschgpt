#include <stdarg.h> 

int io_printf(const char *fmt, ...);
int io_printf_do_not_store(const char *fmt, ...);
void io_timestamp();
void io_app_error(char * str, int length);
void io_server_error(char * str, int length);
void io_str(char * str);
void io_char(char c);
void io_request_info(unsigned int port, int promptTokens, int completionTokens);

bool io_open_history_file(char * filePath);
void io_close_history_file();

void io_clear_screen();


void io_scrollback_init(int max_lines, int max_line_len, int user_entry_rows);

void io_scrollback_free();

void io_scrollback_refresh();

void io_scrollback_scroll(int delta);

void io_scrollback_reset_view_bottom();