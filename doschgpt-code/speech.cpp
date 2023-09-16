#include <dos.h>

#include "speech.h"

/*
This is unofficial API implementation for Text-to-Speech Engine by First Byte.

You can obtain this Engine here (V3, SmoothTalker, Sound Blaster only):
https://winworldpc.com/product/dr-sbaitso/2x

Or there (V4, Text-to-Speech Engine, a lot of other sound devices supported):
https://winworldpc.com/product/monologue/for-dos-v-31

Please note that this file based on disassembled and cleaned code.
There is no documentation on functions so use anything except functions
declared in "speech.h" at your own risk. See "talk.c" code for example.

This code compiled and tested with Borland Turbo C 2.01 (1988):
https://web.archive.org/web/20060614091700/http://bdn.borland.com/article/20841

Comments, questions and suggestions about this API implementation accepted here:
https://github.com/systoolz/dosbtalk

V3 Sound Blaster initialization:
  SBTALKER.EXE /dBLASTER
Required files: SBTALKER.EXE and BLASTER.DRV

V4 Sound Blaster initialization (run in this order):
  SOUNDBST.EXE
  SPEECH.EXE
Required files: SOUNDBST.EXE, SPEECH.EXE, KERNEL.DIC and all V4ENG*.*
Exchange SOUNDBST.EXE with any other sound driver you may want from this list:
SOUNDACP.EXE - Driver for the ACPA card
SOUNDADD.EXE - Driver for the ADLIB card
SOUNDBST.EXE - Driver for the Sound Blaster
SOUNDCMD.EXE - Driver for the Sound Commander (MediaSonic)
SOUNDCVX.EXE - Driver for the Covox Speech Thing and the Hearsay 100, 500, & 1000
SOUNDDIS.EXE - Driver for The Sound Source (Disney)
SOUNDEC2.EXE - Driver for the Echo PC II
SOUNDIBM.EXE - Driver for the IBM Speech Adapter
SOUNDPS1.EXE - Driver for PS/1 Audio Card
SOUNDPWM.EXE - Driver for the standard speaker
SOUNDTHD.EXE - Driver for the Thunder Board
SOUNDTND.EXE - Driver for a Tandy Sound System

Original copyrights follows below.

(READ.EXE)
SOUND BLASTER Text-to-Speech Text Reader Version 1.00
Copyright (c) CREATIVE LABS, INC., 1989-1991. All rights reserved.
Copyright (c) CREATIVE Technology Pte Ltd, 1989-1991. All rights reserved.

(V3)
SmoothTalker (R), Version 3.5, male voice
Copyright (c) 1983-1990 First Byte, All Rights Reserved.
Protected by U.S. Patents #4,692,941 and #4,617,645

(V4)
First Byte Text-to-Speech Engine Version 4.1
Copyright (c) 1991, All Rights Reserved.
Technology contained herein protected by one or more of the following U.S. patents:
#4,692,941 #4,617,645 #4,852,168 #4,805,220 #4,833,718
*/

/* V3 only: taken from READ.EXE code */
enum {
  TTS_V3_FUNC_PARSER = 0,
  TTS_V3_FUNC_UNKNOWN = 1, /* unknown - code for this not present in READ.EXE */
  TTS_V3_FUNC_SET_GLOBALS = 2,
  TTS_V3_FUNC_DICT_INS = 3,
  TTS_V3_FUNC_INIT_DICT = 4,
  TTS_V3_FUNC_DICT_DEL = 5,
  TTS_V3_FUNC_DICT_DUMP = 6,
  TTS_V3_FUNC_SAY = 7,
  TTS_V3_FUNC_SPEAKM = 8,
  TTS_V3_FUNC_SPEAKF = 9
};

/* V4 only: taken from driver SPEECH.EXE code */
enum {
  TTS_V4_FUNC_UNKNOWN_01 = 1,
  TTS_V4_FUNC_UNKNOWN_02 = 2,
  TTS_V4_FUNC_UNKNOWN_03 = 3,
  TTS_V4_FUNC_UNKNOWN_04 = 4,
  TTS_V4_FUNC_PARSER = 5,
  TTS_V4_FUNC_SAY = 6,
  TTS_V4_FUNC_UNKNOWN_06 = 7,
  TTS_V4_FUNC_UNKNOWN_07 = 8,
  TTS_V4_FUNC_SET_GLOBALS = 9,
  TTS_V4_FUNC_SPEAKM = 10,
  TTS_V4_FUNC_SPEAKF = 11,
  TTS_V4_FUNC_UNKNOWN_12 = 12,
  TTS_V4_FUNC_UNKNOWN_13 = 13,
  TTS_V4_FUNC_UNKNOWN_14 = 14,
  TTS_V4_FUNC_UNKNOWN_15 = 15,
  TTS_V4_FUNC_UNKNOWN_16 = 16,
  TTS_V4_FUNC_UNKNOWN_17 = 17,
  TTS_V4_FUNC_UNKNOWN_18 = 18,
  TTS_V4_FUNC_UNKNOWN_19 = 19,
  TTS_V4_FUNC_UNKNOWN_20 = 20
};

enum {
  TTS_GLOB_GENDER = 0,
  TTS_GLOB_TONE = 1,
  TTS_GLOB_VOLUME = 2,
  TTS_GLOB_PITCH = 3,
  TTS_GLOB_SPEED = 4,
  /* below V3 only */
  TTS_GLOB_COUNT = 5,
  TTS_GLOB_ACTION = 6
};

enum {
  TTS_DATA_READ = 0x00,
  TTS_DATA_MASK = 0x7F,
  TTS_DATA_WRITE = 0x80
};

/* argument must be in AL register - it's not actual stack value */
typedef long cdecl (far *LPPLAYFUNCV3)(unsigned char);
/* argument on stack, called function cleans up the stack, SI and DI trashed */
typedef long pascal (far *LPPLAYFUNCV4)(unsigned char);

typedef unsigned char far * FPBYTE;
typedef unsigned short far * FPWORD;

static LPPLAYFUNCV3 fpDrvEntryV3 = (LPPLAYFUNCV3) MK_FP(0, 0); /* DRVR_ENTRY */
static LPPLAYFUNCV4 fpDrvEntryV4 = (LPPLAYFUNCV4) MK_FP(0, 0); /* DRVR_ENTRY */
static FPWORD fpGlob = (FPWORD) MK_FP(0, 0); /* globals */
static FPWORD fpData = (FPWORD) MK_FP(0, 0); /* all data offset */
static short EngineVersion = 0;

/* clip data value in allowed range */
short ClipData(short dval, short dmin, short dmax) {
  dval = (dmin < dval) ? dval : dmin;
  dval = (dmax > dval) ? dval : dmax;
  return(dval);
}

/* handy replacement for MoveTo1() / MoveTo2() / MoveFrom1() / MoveFrom2() */
short BufferData(char *s, unsigned char t, unsigned short l) {
unsigned short i;
FPBYTE fp;
  i = 0;
  if (EngineVersion) {
    if (EngineVersion == 3) {
      /* only 2 buffers */
      fp = (FPBYTE) &fpData[(0x20 + ((t & TTS_DATA_MASK) ? 0x100 : 0)) / 2];
      if (t & TTS_DATA_WRITE) {
        /* write to internal driver buffer */
        for (i = 0; ((i < 255) && (s[i])); i++) {
          fp[i + 1] = s[i];
          /* first byte - buffer length */
          fp[0] = i + 1;
        }
        i = 1;
      } else {
        /* read from internal driver buffer */
        if (l) {
          i = l - 1;
          i = (i < fp[0]) ? i : fp[0];
          while (i--) {
            fp++;
            *s = *fp;
            s++;
          }
          *s = 0;
          i = 1;
        }
      }
    } else {
      /* 5 different buffer pointers seg+off pairs [0..4] */
      i = 4 + (ClipData(t & TTS_DATA_MASK, 0, 4) * 2);
      if (t & TTS_DATA_WRITE) {
        fpData[i] = FP_OFF(s);
        fpData[i + 1] = FP_SEG(s);
        i = 1;
      } else {
        fp = (FPBYTE) MK_FP(fpData[i + 1], fpData[i]);
        i = 0;
        /* read from internal driver buffer */
        if (fp && l) {
          i = l - 1;
          while ((i--) && (*fp)) {
            *s = *fp;
            fp++;
            s++;
          }
          *s = 0;
          i = 1;
        }
      }
    }
  }
  return(i);
}

/* V3 only: unloads driver from memory */
short Unload(void) {
union REGS rs;
short r;
  r = -1;
  if (EngineVersion == 3) {
    rs.x.ax = 0xFBFF;
    int86(0x2F, &rs, &rs);
    r = rs.x.ax;
  }
  return(r);
}

/* V3/V4: detects installed Text-to-Speech driver */
short DetectSpeech(void) {
struct SREGS sr;
union REGS rs;
FPWORD p;
  if (!EngineVersion) {
    /* check multiplex interrupt vector */
    if (_dos_getvect(0x2F)) {
      /* check service */
      rs.x.ax = 0xFBFB;
      segread(&sr);
      int86x(0x2F, &rs, &rs, &sr);
      if (!rs.x.ax) {
        /* pointer to driver data */
        p = (FPWORD) MK_FP(sr.es, rs.x.bx);
        if (
          /* "FB" - First Byte signature */
          (p[0] == 0x4246) && (
            /* API version: 3.x (SmoothTalker) or 4.x (Text-to-Speech Engine) */
            ((p[1] & 0xFF00) == 0x0300) || ((p[1] & 0xFF00) == 0x0400)
          )
        ) {
          /* save data global pointer */
          fpData = p;
          /* save major API version */
          EngineVersion = (p[1] >> 8) & 0xFF;
          /*
          V3 driver header:
            char[2] - signature characters "FB"
            word - driver version (0x03##)
            dword - far pointer to driver entry
            byte[24] - unknown
            char[2][256] - two internal string buffers (first byte string length)
            word[7+] - globals (gender, tone, volume, pitch, speed, count, action, ...)

          V4 driver header:
            char[2] - signature characters "FB"
            word - driver version (0x04##)
            dword - far pointer to driver entry
            dword[5] - far pointers to ASCIIZ string buffers
            word[5+] - globals (gender, tone, volume, pitch, speed, ...)
          */
          if (EngineVersion == 3) {
            /* save driver entry address */
            fpDrvEntryV3 = (LPPLAYFUNCV3) MK_FP(p[3], p[2]);
            /* skip header + 1st buffer + 2nd buffer */
            fpGlob = (FPWORD) MK_FP(sr.es, rs.x.bx + 0x20 + 0x100 + 0x100);
          } else {
            /* save driver entry address */
            fpDrvEntryV4 = (LPPLAYFUNCV4) MK_FP(p[3], p[2]);
            /* skip header + buffer pointers */
            fpGlob = (FPWORD) MK_FP(sr.es, rs.x.bx + 0x1C);
          }
        }
      }
    }
  }
  return(EngineVersion);
}

/* driver caller helper */
long CallDriver(unsigned char id) {
short di_temp, si_temp;
long r;
  r = 0;
  if (EngineVersion) {
    /* save registers */
    // di = _DI;
    // si = _SI;

    _asm {
        mov di_temp, di;
        mov si_temp, si;
    }

    if (EngineVersion == 3) {
      /* V3: xor 0x00 is useless, but this will put function argument to the AL register */
      r = fpDrvEntryV3(id ^ 0x00);
    } else {
      /* V4: argument on stack, called function cleans up the stack, SI and DI trashed */
      r = fpDrvEntryV4(id);
    }
    /* restore registers */
    // _SI = si;
    // _DI = di;

    _asm {
        mov di, di_temp;
        mov si, si_temp;
    }
  }
  return(r);
}

/* write global settings to the driver memory */
void UseGlobals(short gen, short ton, short vol, short pit, short spd) {
  if (EngineVersion) {
    if (gen >= 0) { fpGlob[TTS_GLOB_GENDER] = gen; } /* unused */
    /* tone: 0 - bass; 1 - tremble */
    if (ton >= 0) { fpGlob[TTS_GLOB_TONE] = ClipData(ton, 0, 1); } /* unused */
    if (vol >= 0) { fpGlob[TTS_GLOB_VOLUME] = ClipData(vol, 0, 9); }
    if (pit >= 0) { fpGlob[TTS_GLOB_PITCH] = ClipData(pit, 0, 9); }
    if (spd >= 0) { fpGlob[TTS_GLOB_SPEED] = ClipData(spd, 0, 9); }
  }
}

/* V3/V4: set driver globals for playback, use -1 to keep current values */
short SetGlobals(short gen, short ton, short vol, short pit, short spd) {
short r;
  r = 0;
  if (EngineVersion) {
    UseGlobals(gen, ton, vol, pit, spd);
    r = (short) CallDriver((EngineVersion == 3) ? TTS_V3_FUNC_SET_GLOBALS : TTS_V4_FUNC_SET_GLOBALS);
  }
  return(r);
}

/* V3/V4: get phonetical representation of the English text string */
short Parser(char *engstr, char *phonstr, unsigned short count) {
short r;
  r = -1;
  if (EngineVersion) {
    if (EngineVersion == 3) {
      /* first syllable count, non-zero value reduces output string start */
      fpGlob[TTS_GLOB_COUNT] = 0;
    }
    BufferData(engstr, TTS_DATA_WRITE | 0, 0);
    r = (short) CallDriver((EngineVersion == 3) ? TTS_V3_FUNC_PARSER : TTS_V4_FUNC_PARSER);
    BufferData(phonstr, TTS_DATA_READ | 1, count);
  }
  return(r);
}

/* V3 only: unknown, untested (dictionary initialization?) */
short InitDict(short act) {
short r;
  r = 0;
  if (EngineVersion == 3) {
    fpGlob[TTS_GLOB_ACTION] = act;
    r = (short) CallDriver(TTS_V3_FUNC_INIT_DICT);
  }
  return(r);
}

/* V3 only: unknown, untested (dictionary insertion?) */
short DictIns(char *engstr, char *phonstr) {
short r;
  r = 0;
  if (EngineVersion == 3) {
    BufferData(engstr, TTS_DATA_WRITE | 0, 0);
    BufferData(phonstr, TTS_DATA_WRITE | 1, 0);
    r = (short) CallDriver(TTS_V3_FUNC_DICT_INS);
  }
  return(r);
}

/* V3 only: unknown, untested (dictionary deletion?) */
short DictDel(char *engstr) {
short r;
  r = 0;
  if (EngineVersion == 3) {
    BufferData(engstr, TTS_DATA_WRITE | 0, 0);
    r = (short) CallDriver(TTS_V3_FUNC_DICT_DEL);
  }
  return(r);
}

/* V3 only: unknown, untested (dictionary dumper?) */
/* returns long in DX:AX pair */
long DictDump(char *engstr, char *phonstr) {
long r;
  r = 0;
  if (EngineVersion == 3) {
    BufferData(engstr, TTS_DATA_WRITE | 0, 0);
    BufferData(phonstr, TTS_DATA_WRITE | 1, 0);
    r = CallDriver(TTS_V3_FUNC_DICT_DUMP);
  }
  return(r);
}

/* say text with current global voice settings defined by SetGlobals()
in V3 driver max text length limited to 255 bytes */
short Say(char *engstr) {
short r;
  r = 0;
  if (EngineVersion) {
    BufferData(engstr, TTS_DATA_WRITE | 0, 0);
    r = (short) CallDriver((EngineVersion == 3) ? TTS_V3_FUNC_SAY : TTS_V4_FUNC_SAY);
  }
  return(r);
}

/* male? use Parser() to get phonetical representation of the text for phonstr
V3/V4: works the same as SetGlobals() + Say() pair */
short SpeakM(char *phonstr, short ton, short vol, short pit, short spd) {
short r;
  r = 0;
  if (EngineVersion) {
    UseGlobals(-1, ton, vol, pit, spd);
    /* bug in READ.EXE code (V3) - text was misplaced into second buffer here */
    BufferData(phonstr, TTS_DATA_WRITE | 0, 0);
    r = (short) CallDriver((EngineVersion == 3) ? TTS_V3_FUNC_SPEAKM : TTS_V4_FUNC_SPEAKM);
  }
  return(r);
}

/* female? use Parser() to get phonetical representation of the text for phonstr
V3/V4: works the same as SetGlobals() + Say() pair */
short SpeakF(char *phonstr, short ton, short vol, short pit, short spd) {
short r;
  r = 0;
  if (EngineVersion) {
    UseGlobals(-1, ton, vol, pit, spd);
    /* bug in READ.EXE code (V3) - text was misplaced into second buffer here */
    BufferData(phonstr, TTS_DATA_WRITE | 0, 0);
    r = (short) CallDriver((EngineVersion == 3) ? TTS_V3_FUNC_SPEAKF : TTS_V4_FUNC_SPEAKF);
  }
  return(r);
}

/* V3 only: code taken from Set-echo.exe */
short SetEcho(short parm) {
FPWORD fp;
short r;
  r = 0;
  if (EngineVersion == 3) {
    fp = (FPWORD) _dos_getvect(0xF3);
    if (fp) {
      /* program help says from 0 to 4000 but actual code clip it at 3950 */
      *fp = ClipData(parm, 0, 3950);
      r = 1;
    }
  }
  return(r);
}

/* restore default settings */
short ResetSpeech(void) {
short r;
  r = 0;
  if (EngineVersion) {
    /* reset to default settings */
    if (EngineVersion == 3) {
      SetGlobals(0, 0, 5, 5, 5);
      /* V3 only: turn off echo */
      SetEcho(0);
    } else {
      /* V4 only: higher pitch (5 -> 7) in default settings */
      SetGlobals(0, 0, 5, 7, 5);
    }
    /* for whatever reason Set-echo.exe do this (clears/reinit sound buffer?) */
    Say(". ");
    r = 1;
  }
  return(r);
}

/* return full Engine version */
short SpeechVersion(void) {
  return(EngineVersion ? fpData[1] : 0);
}