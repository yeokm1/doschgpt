/*

   mTCP Inlines.H
   Copyright (C) 2006-2023 Michael B. Brutman (mbbrutman@gmail.com)
   mTCP web page: http://www.brutman.com/mTCP


   This file is part of mTCP.

   mTCP is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   mTCP is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with mTCP.  If not, see <http://www.gnu.org/licenses/>.


   Description: Inline functions I find useful

   Changes:

   2015-01-17: Split out from Utils.H with functions consolidated from various
               other places.

*/


#ifndef _INLINES_H
#define _INLINES_H

#include "types.h"




//-----------------------------------------------------------------------------
//
// Macros to convert from host byte order to/from network byte order.

extern uint16_t htons( uint16_t );
#pragma aux htons = \
  "xchg al, ah"     \
  parm [ax]         \
  modify [ax]       \
  value [ax];

#define ntohs( x ) htons( x )


extern uint32_t htonl( uint32_t );
#pragma aux htonl = \
  "xchg al, ah"     \
  "xchg bl, bh"     \
  "xchg ax, bx"     \
  parm [ax bx]      \
  modify [ax bx]    \
  value [ax bx];

#define ntohl( x ) htonl( x )




//-----------------------------------------------------------------------------
//
// DOS specific functions

// Returns the current DOS version.  Major is in the low byte, minor is in
// the high byte.

extern uint16_t dosVersion( void );
#pragma aux dosVersion = \
  "mov ah,0x30"          \
  "int 0x21"             \
  modify [ax]            \
  value [ax];


// Returns number of 16 byte paragraphs available.  So multiply the
// return value by 16 to get a real byte count.
//
// Purposefully ask for more memory than is available so that it fails.
// The amount of memory available will be the return code.

extern uint16_t getFreeDOSMemory( void );
#pragma aux getFreeDOSMemory = \
  "mov bx, 0FFFFh"  \
  "mov ah, 48h"     \
  "int 21h"         \
  "clc"             \
  modify [ax bx]    \
  value [bx];


// Calling the runtime to do a stat( ) call brings in a lot of new code.
// Use a DOS interrupt directly to avoid that bloat.
//
// int 21h, ax=4300 = Get File Attributes
//   ds:dx is the Asciiz filename
//   On error CF is set and AX has the error code
//   If good, CX as the file attributes:
//     7: shareable       3: volume label
//     6: not used        2: system
//     5: archive         1: hidden
//     4: directory       0: read-only
//
// Return 0 if good and attributes in attrs, otherwise return 1.
// Does not like it if the name ends in a backslash.

extern uint8_t getFileAttributes( const char far * name, uint16_t *attrs );
#pragma aux getFileAttributes = \
  "mov ax, 4300h"               \
  "int 21h"                     \
  "lahf"                        \
  "and ah, 1h"                  \
  "mov es:bx, cx"               \
  parm [ds dx] [es bx]          \
  value [ah];




//-----------------------------------------------------------------------------
//
// Screen handling functions
//
// All coordinates are zero based.
// push/pop bp is a protection against old buggy BIOSes.


// getEgaMemSize
//
// Used primarily to determine if this is an EGA card.  If you call it and
// bl is returned to you as 0x10, then this is not an ega card.  A real EGA
// card would have used the low nibble to tell you how much memory is on
// the card.
extern uint8_t getEgaMemSize( void );
#pragma aux getEgaMemSize = \
  "push bp"                 \
  "mov ah, 12h"             \
  "mov bl, 10h"             \
  "int 10h"                 \
  "pop bp"                  \
  modify [bh bl ch cl ah]   \
  value [ bl ]


extern void turnOffEgaBlink( void );
#pragma aux turnOffEgaBlink = \
  "push bp"       \
  "mov ax, 1003h" \
  "mov bx, 0h"    \
  "int 10h"       \
  "pop bp"


extern void setVideoMode( uint8_t );
#pragma aux setVideoMode = \
  "push bp"    \
  "mov ah, 0"  \
  "int 10h"    \
  "pop bp"     \
  parm [al]    \
  modify [ax];


// If you pass a 0 to lines the screen gets cleared.

extern void scrollScreen( uint8_t lines, uint8_t hiRow, uint8_t hiCol, uint8_t lowRow, uint8_t lowCol );
#pragma aux scrollScreen = \
  "push bp"             \
  "mov ah, 06h"         \
  "mov bh, 7h"          \
  "int 10h"             \
  "pop bp"              \
  parm [al] [ch] [cl] [dh] [dl] \
  modify [ah bh];


extern void writeChar( uint8_t col, uint8_t row, uint8_t ch, uint8_t attr );
#pragma aux writeChar = \
  "push bp"          \
  "push ax"          \
  "push bx"          \
  "mov ah, 2h"       \
  "mov bh, 0h"       \
  "int 10h"          \
  "pop bx"           \
  "pop ax"           \
  "mov ah, 9h"       \
  "mov bh, 0h"       \
  "mov cx, 1h"       \
  "int 10h"          \
  "pop bp"           \
  parm [dl] [dh] [al] [bl]    \
  modify [ax bx cx dx];



extern unsigned char wherex( void );
#pragma aux wherex = \
  "push bp"          \
  "mov ah, 3h"       \
  "mov bh, 0h"       \
  "int 10h"          \
  "pop bp"           \
  modify [ ax bx cx dx ] \
  value [ dl ];


extern unsigned char wherey( void );
#pragma aux wherey = \
  "push bp"          \
  "mov ah, 3h"       \
  "mov bh, 0h"       \
  "int 10h"          \
  "pop bp"           \
  modify [ ax bx cx dx ] \
  value [ dh ];


extern void gotoxy( unsigned char col, unsigned char row );
#pragma aux gotoxy = \
  "push bp"          \
  "mov ah, 2h"       \
  "mov bh, 0h"       \
  "int 10h"          \ 
  "pop bp"           \
  parm [dl] [dh]     \
  modify [ax bh dx];


extern void setBlockCursor( void );
#pragma aux setBlockCursor = \
  "push bp"         \
  "mov ah, 1h"      \
  "mov cx, 0x000Fh" \
  "int 10h"         \
  "pop bp"          \
  modify [ah cx];


extern void hideCursor( void );
#pragma aux hideCursor = \
  "push bp"         \
  "mov ah, 1h"      \
  "mov cx, 0x202Fh" \
  "int 10h"         \
  "pop bp"          \
  modify [ah cx];


// Generic video interrupt call that allows for two parameters (AX and BX)
// and returns a value in AX.
extern uint16_t videoInt( uint16_t, uint16_t );
#pragma aux videoInt = \
  "push bp"      \
  "int 10h"      \
  "pop bp"       \
  parm [ax] [bx] \
  value [ax];


// writeCharWithoutSnow
//   base is the screen base
//   offset is the offset into the frame buffer to write to.
//   c is the character and attribute to write.

extern void writeCharWithoutSnow( uint16_t base, uint16_t off, uint16_t c );
#pragma aux writeCharWithoutSnow = \
  "push es"                  \
  "mov es,ax"                \
  "mov dx,0x3da"             \
  "cli"                      \
  "in_retrace: in al,dx"     \
  "test al,1"                \
  "jnz in_retrace"           \
  "retrace: in al,dx"        \
  "shr al,1"                 \
  "jnc retrace"              \
  "xchg bx,ax"               \
  "stosw"                    \
  "sti"                      \
  "pop es"                   \
  parm [ax] [di] [bx]            \
  modify [dx];


extern void waitForCGARetraceLong( void );
#pragma aux waitForCGARetraceLong = \
  "mov dx,0x3da"             \
  "in_retrace: in al,dx"     \
  "and al, 8"                \
  "jnz in_retrace"           \
  "retrace: in al,dx"        \
  "and al, 8"                \
  "jz retrace"               \
  modify [dx al];





//-----------------------------------------------------------------------------
//
// Keyboard handling functions
//
// Watcom bioskey routines seem to lock up VirtualBox when entering a character
// using Alt-Numpad keyboard codes.  These are equivalent and even though
// VirtualBox 6.x doesn't let you enter characters that way, these don't freeze
// the keyboard handler up.

// This could be a few instructions shorter by setting bh to zero before doing
// the interrupt, but this is safer if a bad bios modifies bh.

extern bool biosIsKeyReady( void );
#pragma aux biosIsKeyReady = \
  "mov ah, 0x1"        \
  "int 0x16"           \
  "jz nokey"           \
  "mov bh, 0x1"        \
  "jmp end"            \
  "nokey: mov bh, 0x0" \
  "end:"               \
  modify [ax bh]       \
  value [bh];

extern uint16_t biosKeyRead( void );
#pragma aux biosKeyRead = \
  "mov ah, 0x0"        \
  "int 0x16"           \
  modify [ax]          \
  value [ax];





// This fills a region of memory a word at a time very efficiently.  Usually
// used with painting portions of the screen, but it really is more generic.
//
// For better performance the starting address should be word aligned.

#if defined(M_I86CM) || defined(M_I86LM)

extern void fillUsingWord( void far * target, uint16_t fillWord, uint16_t len );
#pragma aux fillUsingWord = \
  "push es"    \
  "push di"    \
  "mov es, dx" \
  "mov di, bx" \
  "rep stosw"  \
  "pop di"     \
  "pop es"     \
  modify [ax]  \
  parm [dx bx] [ax] [cx]

#endif



#endif

