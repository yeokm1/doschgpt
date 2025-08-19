#include <stdio.h>
#include <time.h>
#include <string.h>
#include <dos.h>
#include <stdlib.h>
#include <i86.h>
#include "textio.h"
#include "dbgserial.h"

#define BIOS_INT  int86
#define REGS_T    union REGS

#define TIMESTAMP_FORMAT "%Y-%m-%d %H:%M:%S"

#define TIMESTAMP_SIZE 30

char timestampStr[TIMESTAMP_SIZE];

FILE *historyFile = NULL;



static void sb_push_line(const char* s);


int io_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char temp[160];

    vsnprintf(temp, sizeof(temp), fmt, ap);

    int length_of_string = strlen(temp);

    if(temp[length_of_string - 1] == '\n'){
        //Remove trailing newline
        temp[length_of_string - 1] = '\0';
    }

    sb_push_line(temp);

    va_end(ap);
    
    fflush(stdout);
    return 0;
}

int io_printf_do_not_store(const char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    char temp[90];
    vsnprintf(temp, sizeof(temp), fmt, ap);
    int result = printf("%s", temp);
    va_end(ap);
    fflush(stdout);
    return result;
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

// From ChatGPT
int getScreenRows(void) {
    unsigned char far* bda_rowsm1 = (unsigned char far*)MK_FP(0x40, 0x84);
    unsigned char v = *bda_rowsm1;          // 0 if unknown (e.g., MDA/CGA)
    return v ? (v + 1) : 25;                // fall back to 25
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

void io_str(char * str){

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

    //io_printf("\n");


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

void io_clear_screen() {
    union REGS in, out;

    fflush(stdout);

    // Get columns and active page (works everywhere)
    in.x.ax = 0x0F00;               // AH=0Fh: get video mode; AH=cols, BH=page
    int86(0x10, &in, &out);
    unsigned char cols = getScreenColumns();
    unsigned char page = out.h.bh;

    // Rows: assume 25 (safe for MDA/CGA). If EGA/VGA sets BDA rows-1 at 0x40:0x0084, use it.
    unsigned char rows = getScreenRows();
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

static char **sb_lines        = NULL;  // ring of pointers to lines
static int    sb_capacity     = 0;     // total lines in ring
static int    sb_max_line_len = 0;     // max chars per stored line (soft cap)
static int    sb_head         = 0;     // next write index
static long   sb_view_delta   = 0;
static int    sb_rows         = 25 - 3; // visible rows; keep simple
static int    sb_cols         = 80;    // cache columns for wrapping
static long   sb_count        = 0;     // total lines ever stored

void io_scrollback_init(int max_lines, int max_line_len, int user_entry_rows){

    sb_cols = getScreenColumns();

    // Minus extra 1 for the "Me:" row
    sb_rows = getScreenRows() - user_entry_rows - 1;

    sb_capacity     = max_lines;
    sb_max_line_len = max_line_len;
    sb_lines = (char**) calloc(sb_capacity, sizeof(char*));
    for(int i=0; i<sb_capacity; i++){
        sb_lines[i] = (char*) calloc(sb_max_line_len, 1);
    }
    sb_head     = 0;
    sb_view_delta = 0;
    // sb_follow   = 1; // start following
}

void io_scrollback_free(){
    if(!sb_lines) return;
    for(int i=0;i<sb_capacity;i++){
        free(sb_lines[i]);
    }
    free(sb_lines);
    sb_lines = NULL;
    sb_capacity = 0;
}



// Optional: if you already have these, you can reuse them.
static void set_cursor_rc(unsigned char row, unsigned char col){
    REGS_T r = {0};
    r.h.ah = 0x02; r.h.bh = 0; r.h.dh = row; r.h.dl = col;
    BIOS_INT(0x10, &r, &r);
}

static void sb_push_line(const char* s){
    if(!sb_lines) return;
    int len = strlen(s);

    // Clip long lines
    if(len >= sb_max_line_len) len = sb_max_line_len - 1;

    memset(sb_lines[sb_head], 0, sb_max_line_len);
    memcpy(sb_lines[sb_head], s, len);

    dbgserial_printf("%s", sb_lines[sb_head]);


    sb_head = (sb_head + 1) % sb_capacity;

    sb_view_delta = 0;
    sb_count++;



    dbgserial_printf("new sb_count %d", sb_count);
    dbgserial_printf("new sb head %d",  sb_head);

    //TODO: Make extra long lines into second line

}

// Helper: positive modulo for ring indices
static inline int wrap_idx(int x, int m){
    int r = x % m;
    return (r < 0) ? (r + m) : r;
}

void io_scrollback_refresh(){
    if (!sb_lines) return;

    io_clear_screen();               // clear everything
    // sb_cols = getScreenColumns(); // if you want to re-read width here

    // How many lines are currently retained in the ring?
    long kept = (sb_count < sb_capacity) ? sb_count : sb_capacity;

    // How many rows we will actually paint
    int rows_to_print = (kept < sb_rows) ? (int)kept : sb_rows;

    if (rows_to_print > 0) {
        // Index of the newest line in the ring
        // (sb_head is the NEXT write slot, so newest is sb_head-1)
        int newest = wrap_idx(sb_head - 1, sb_capacity);

        // sb_view_delta: 0 = bottom (end at newest), -k = scrolled up by k lines
        int last_vis = wrap_idx(newest + sb_view_delta, sb_capacity);

        // First visible index is last_vis - (rows_to_print - 1)
        int first_vis = wrap_idx(last_vis - (rows_to_print - 1), sb_capacity);

        // Paint exactly rows_to_print lines from first_vis forward
        int idx = first_vis;
        for (int row = 0; row < rows_to_print; ++row) {
            // truncate to sb_cols if needed
            int n = 0; while (sb_lines[idx][n] && n < sb_cols) n++;
            fwrite(sb_lines[idx], 1, n, stdout);
            fputc('\n', stdout);

            idx++; if (idx == sb_capacity) idx = 0;
        }
    }

    // Fill any remaining history rows with blank lines so the input sits
    // exactly after the history area
    for (int row = rows_to_print; row < sb_rows; ++row) {
        fputc('\n', stdout);
    }

    // Fixed input prompt at the bottom of the history area
    fputs("Me:\n", stdout);
    fflush(stdout);
}

void io_scrollback_scroll(int delta){
    if (!sb_lines) return;

    // how many lines exist in the ring right now?
    long kept = (sb_count < sb_capacity) ? sb_count : sb_capacity;

    // nothing to scroll if we don't fill at least one screen
    if (kept <= sb_rows) {
        dbgserial_printf("scroll: kept=%ld <= rows=%d; no-op", kept, sb_rows);
        return;
    }

    // desired new delta (relative to bottom)
    long new_delta = (long)sb_view_delta + (long)delta;

    // bottom clamp (can't go below bottom)
    if (new_delta > 0) new_delta = 0;

    // top clamp: max lines we can scroll up is kept - sb_rows
    long min_delta = - ( (long)kept - (long)sb_rows ); // negative or zero
    if (min_delta > 0) min_delta = 0;                  // safety (shouldn't happen)

    if (new_delta < min_delta) new_delta = min_delta;

    dbgserial_printf("scroll: kept=%ld rows=%d old=%d delta=%d -> new=%ld (min=%ld)\r\n",
                      kept, sb_rows, sb_view_delta, delta, new_delta, min_delta);

    sb_view_delta = (int)new_delta;
    io_scrollback_refresh();
}

void io_scrollback_reset_view_bottom(){
    if(!sb_lines) return;

    sb_view_delta = 0;

    io_scrollback_refresh();
}

