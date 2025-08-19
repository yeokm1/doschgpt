#include "dbgserial.h"
#include <stdio.h>
#include <stdarg.h>
#include <i86.h>

#define BIOS_INT  int86
#define REGS_T    union REGS

static bool serial_enabled = false;

void dbgserial_init_9600_8N1(int com_number){
    REGS_T r = {0};
    r.h.ah = 0x00;
    r.h.al = 0xE3;       // 9600 baud, 8N1 (BIOS bitfield: 1110 0011)
    r.x.dx = (unsigned short)(com_number - 1);  // 0..3 => COM1..COM4
    BIOS_INT(0x14, &r, &r);
    serial_enabled = true;
}
void dbgserial_putc(char ch){

    if(!serial_enabled) return;

    REGS_T r = {0};
    r.h.ah = 0x01; 
    r.h.al = (unsigned char)ch; 
    r.x.dx = 0;
    BIOS_INT(0x14,&r,&r);
}

void dbgserial_printf(const char* fmt, ...){

    if(!serial_enabled) return;

    va_list ap;
    va_start(ap, fmt);
    char temp[160];

    vsnprintf(temp, sizeof(temp), fmt, ap);

    char * temp_ptr = temp;

    while(*temp_ptr) {
        dbgserial_putc(*temp_ptr++);
    }

    dbgserial_putc('\r'); 
    dbgserial_putc('\n');

    va_end(ap);
}

void dbgserial_close(){
    serial_enabled = false;
}