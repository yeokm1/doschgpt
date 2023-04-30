#include <stdio.h>
#include <time.h>
#include <string.h>

#define TIMESTAMP_FORMAT "%Y-%m-%d %H:%M:%S"

#define TIMESTAMP_SIZE 30

char timestampStr[TIMESTAMP_SIZE];

FILE *historyFile = NULL;

void updateTimeStamp(){
    time_t currentTime;
    struct tm* timeInfo;
    time(&currentTime);
    timeInfo = localtime(&currentTime);

    memset(timestampStr, 0, TIMESTAMP_SIZE);
    strftime(timestampStr, TIMESTAMP_SIZE, TIMESTAMP_FORMAT, timeInfo);
}

void io_timestamp(){
    updateTimeStamp();

    #define TIMESTAMP_PRINT_FORMAT "[%s]\n"
    printf(TIMESTAMP_PRINT_FORMAT, timestampStr);

    if(historyFile){
        fprintf(historyFile, TIMESTAMP_PRINT_FORMAT, timestampStr);
    }
    
}

void io_app_error(char * str, int length){

    #define APP_ERROR_FORMAT "App Error:\n%.*s\n"
    printf(APP_ERROR_FORMAT, length, str);

    if(historyFile){
        fprintf(historyFile, APP_ERROR_FORMAT, length, str);
    }
}

void io_server_error(char * str, int length){
    #define GPT_ERROR_FORMAT "Server Error:\n%.*s\n"
    printf(GPT_ERROR_FORMAT, length, str);

    if(historyFile){
        fprintf(historyFile, GPT_ERROR_FORMAT, length, str);
    }
}

void io_str_newline(char * str){
    printf("%s\n", str);

    if(historyFile){
        fprintf(historyFile, "%s\n", str);
    }
}

void io_str_len(char * str, int length){
    printf("%.*s", length, str);

    if(historyFile){
        fprintf(historyFile, "%.*s", length, str);
    }
}

void io_write_str_no_print(char * str, int length){
    if(historyFile){
        fprintf(historyFile, "%.*s", length, str);
    }
}

void io_char(char c){
    printf("%c", c);

    if(historyFile){
        fprintf(historyFile, "%c", c);
    }
}

void io_request_info(unsigned int port, int promptTokens, int completionTokens){

    #define INFO_FORMAT "[Outgoing port %u, %d prompt tokens, %d completion tokens]\n"

    printf(INFO_FORMAT, port, promptTokens, completionTokens);

    if(historyFile){
        fprintf(historyFile, INFO_FORMAT, port, promptTokens, completionTokens);
    }
}

bool io_open_history_file(char * filePath){
    historyFile = fopen(filePath, "a");

    if(historyFile == NULL){
        return false;
    } else {
        return true;
    }
}

void io_close_history_file(){
    if(historyFile != NULL){
        fclose(historyFile);
        historyFile = NULL;
    }
}


