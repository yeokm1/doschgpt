#include <stdio.h>
#include <time.h>
#include <string.h>
#include <dos.h>
#include "textio.h"


#define TIMESTAMP_FORMAT "%Y-%m-%d %H:%M:%S"

#define TIMESTAMP_SIZE 30

char timestampStr[TIMESTAMP_SIZE];

FILE *historyFile = NULL;

static int io_vprintf(const char *fmt, va_list ap) {
    int result = vprintf(fmt, ap);
    return result;
}

int io_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = io_vprintf(fmt, ap);
    va_end(ap);
    return r;
}

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
    io_printf(TIMESTAMP_PRINT_FORMAT, timestampStr);

    if(historyFile){
        fprintf(historyFile, TIMESTAMP_PRINT_FORMAT, timestampStr);
    }
    
}

void io_app_error(char * str, int length){

    #define APP_ERROR_FORMAT "App Error:\n%.*s\n"
    io_printf(APP_ERROR_FORMAT, length, str);

    if(historyFile){
        fprintf(historyFile, APP_ERROR_FORMAT, length, str);
    }
}

void io_server_error(char * str, int length){
    #define GPT_ERROR_FORMAT "Server Error:\n%.*s\n"
    io_printf(GPT_ERROR_FORMAT, length, str);

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
            io_printf("%.*s", charactersRemaining, str + startPos);
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

        io_printf("%.*s\n", lengthToPrint, str + startPos);

        startPos = endPosOfCurrentString + 1;
    }

    io_printf("\n");


    //io_printf("%s\n", str);

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
    io_printf("%c", c);

    if(historyFile){
        fprintf(historyFile, "%c", c);
    }
}

void io_request_info(unsigned int port, int promptTokens, int completionTokens){

    #define INFO_FORMAT "[Outgoing port %u, %d prompt tokens, %d completion tokens]\n"

    io_printf(INFO_FORMAT, port, promptTokens, completionTokens);

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

void io_clear_screen(void) {
    union REGS in, out;

    fflush(stdout);

    // Get columns and active page (works everywhere)
    in.x.ax = 0x0F00;               // AH=0Fh: get video mode; AH=cols, BH=page
    int86(0x10, &in, &out);
    unsigned char cols = getScreenColumns();
    unsigned char page = out.h.bh;

    // Rows: assume 25 (safe for MDA/CGA). If EGA/VGA sets BDA rows-1 at 0x40:0x0084, use it.
    unsigned char rows = 25;
#ifndef __386__  // far pointers are only valid in real-mode small/medium models
    {
        volatile unsigned char far* bda_rowsm1 = (unsigned char far*)MK_FP(0x40, 0x84);
        unsigned char v = *bda_rowsm1;
        if (v) rows = (unsigned char)(v + 1);
    }
#endif

    // Clear entire window via AH=06h. Use a fixed attribute (0x07) to avoid AH=08h.
    in.h.ah = 0x06;   // scroll up / clear
    in.h.al = 0x00;   // clear window
    in.h.bh = 0x07;   // fill attribute: white on black
    in.h.ch = 0x00;   // upper-left row
    in.h.cl = 0x00;   // upper-left col
    in.h.dh = (rows ? rows - 1 : 24);            // lower-right row
    in.h.dl = (cols ? cols - 1 : 79);            // lower-right col
    int86(0x10, &in, &out);

    // Home the cursor on the active page
    in.h.ah = 0x02;   // set cursor position
    in.h.bh = page;
    in.h.dh = 0;
    in.h.dl = 0;
    int86(0x10, &in, &out);

    fflush(stdout);
}