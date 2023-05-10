#include <stdio.h>
#include <time.h>
#include <string.h>
#include <dos.h>

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

//Reference https://www.equestionanswers.com/c/c-int86-dos-bios-system-interrupts.php
int getScreenColumns(){

    union REGS input_regs, output_regs;
    int numCols;

    input_regs.h.ah = 0x0F;
    int86(0x10, &input_regs, &output_regs);
    numCols = output_regs.h.ah;

    return numCols;
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

    //First part of this function will do word wrapping

    int columns = getScreenColumns();

    int len = strlen(str);

    int startPos = 0;
    int i;

    while (startPos < len) {

        int charactersRemaining = len - startPos;

        // Find and print the last chunk
        if((charactersRemaining) <= columns){
            printf("%.*s", charactersRemaining, str + startPos);
            break;
        }

        int endOfLine = startPos + columns - 1;

        int endPosOfCurrentString = endOfLine;

        //Look for newlines so we can break prematurely
        if(char * charPos = (char *) memchr(str + startPos, '\n', columns - 1)){

            endPosOfCurrentString = (int) (charPos - str);

        } else {
            // Move backwards until we find a space
            for(int i = endOfLine; i > startPos; i--){
                char currentChar = str[i];

                if(currentChar == ' '){
                    endPosOfCurrentString = i;
                    break;
                }
            }
        }



        int lengthToPrint = endPosOfCurrentString - startPos;

        printf("%.*s\n", lengthToPrint, str + startPos);

        startPos = endPosOfCurrentString + 1;
    }

    printf("\n");


    //printf("%s\n", str);

    //This part writes the non-wrapped portion to file.
    if(historyFile){
        fprintf(historyFile, "%s\n", str);
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


