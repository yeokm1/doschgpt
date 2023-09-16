#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "speech.h"

#define READ_STR_FORMAT "read %.*s"
#define READ_SIZE_BUFF 256

#define MAX_TO_READ 250

char phraseToRead[READ_SIZE_BUFF];

int sbtts_run_command(char * command, bool redirectStdout){

    if(redirectStdout){
        // Send stdout to nul
        freopen("nul", "r", stdout);
    }
    
    int returnCode = system(command);

    if(redirectStdout){
        fflush(stdout);

        //Restore the stdout
        freopen("con", "a", stdout);
    }

    return returnCode;
    
}

bool sbtts_init(){
    sbtts_run_command("SBTALKER /dBLASTER", false);

    if(DetectSpeech()){
        ResetSpeech();
        return true;
    } else {
        return false;
    }
}
 
void sbtts_end(){
    ResetSpeech();
    sbtts_run_command("REMOVE", false);
}

void sbtts_read_this_phrase(char * phrase, int length, bool redirectStdout){
    //Clear buffer before starting every new phrase
    ResetSpeech();
    //Set appropriate speed
    SetGlobals(0, 0, 5, 5, 3);
    
    memset(phraseToRead, 0, READ_SIZE_BUFF);
    memcpy(phraseToRead, phrase, length);
    
    Say(phraseToRead);
}

void sbtts_read_str(char * str_to_read, int length, bool redirectStdout){

    int currentStartPointer = 0;

    if(length < MAX_TO_READ){
        sbtts_read_this_phrase(str_to_read, length, redirectStdout); 
    } else {
        //Look for first break
        for(int i = 0; i < length; i++){
            char currentChar = str_to_read[i];
            int currentLength = i - currentStartPointer;

            if(currentLength > MAX_TO_READ){
                //Since we reach the end before any punctuation, we should backtrack to find a space to avoid breaking up word.
                int j;
                for(j = i; i > currentStartPointer; j--){
                    char candidateSpace = str_to_read[j];
                    if(candidateSpace == ' '){
                        break;
                    }
                }

                i = j;

                sbtts_read_this_phrase(str_to_read + currentStartPointer, i - currentStartPointer, redirectStdout);            
                currentStartPointer = i;

            } else {
                // Split along the punctuation
                if('!' <= currentChar && currentChar <= '/' 
                || ':' <= currentChar && currentChar <= '@' 
                || '[' <= currentChar && currentChar <= '`' 
                || '{' <= currentChar && currentChar <= '~'){

                    //printf("start %d length %d stop at %c\n", currentStartPointer, i - currentStartPointer, currentChar);
                    sbtts_read_this_phrase(str_to_read + currentStartPointer, i - currentStartPointer, redirectStdout);            
                    currentStartPointer = i;      
                }
            }

        }
    }


}