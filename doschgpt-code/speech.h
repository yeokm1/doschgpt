#ifndef __SPEECH_H
#define __SPEECH_H

#ifdef __cplusplus
extern "C" {
#endif

short DetectSpeech(void);
short SetGlobals(short gen, short ton, short vol, short pit, short spd);
short Parser(char *engstr, char *phonstr, unsigned short count);
short Say(char *engstr);
short SetEcho(short parm);
short ResetSpeech(void);
short SpeakM(char *phonstr, short ton, short vol, short pit, short spd);
short SpeakF(char *phonstr, short ton, short vol, short pit, short spd);
short SpeechVersion(void);

#ifdef __cplusplus
}
#endif

#endif
