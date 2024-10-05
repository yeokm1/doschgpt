#include <stdio.h>
#include <stdlib.h>
#include <bios.h>
#include <string.h>

#include "network.h"
#include "utf2cp.h"
#include "textio.h"
#include "sound.h"

#define VERSION "0.17"

#define DOS_CHATGPT_WELCOME_MSG "Welcome to DOS ChatGPT client"
#define DOS_HUGGING_FACE_WELCOME_MSG "Welcome to DOS Hugging Face client"

#define DOS_CHATGPT_WELCOME_SND "Welcome to dos Chat G P T client"
#define DOS_HUGGING_FACE_WELCOME_SND "Welcome to dos Hugging Face client"

#define GOODBYE_SND "Good bye!"

// User configuration
#define CONFIG_FILENAME_DEFAULT "doschgpt.ini"
#define API_KEY_LENGTH_MAX 200
#define MODEL_LENGTH_MAX 100
#define PROXY_HOST_MAX 100
#define CONV_HISTORY_PATH_SIZE 256
#define CONFIG_PATH_SIZE 256

#define MESSAGE_SIZE 5000

char config_apikey[API_KEY_LENGTH_MAX];
char config_model[MODEL_LENGTH_MAX];
float config_req_temperature;
char config_proxy_hostname[PROXY_HOST_MAX];
int config_proxy_port;
uint16_t config_outgoing_start_port;
uint16_t config_outgoing_end_port;
uint16_t config_socketConnectTimeout;
uint16_t config_socketResponseTimeout;

// Command Line Configuration
bool debug_showRequestInfo = false;
bool debug_showRawReply = false;
bool debug_showTimeStamp = false;
int codePageInUse = CODE_PAGE_437;
bool convHistoryGiven = false;
char convHistoryPath[CONV_HISTORY_PATH_SIZE];
bool sound_blaster_tts = false;

bool configPathGiven = false;
char configPath[CONFIG_PATH_SIZE];

bool huggingFace = false;

// Message Request
#define SIZE_MSG_TO_SEND 4096
char * messageToSendToNet;

#define SIZE_MESSAGE_IN_BUFFER 1600
char * messageInBuffer;

#define REPLY_DISPLAY_SIZE 5000
char * replyDisplayBuffer = NULL;
int replyDisplayPos = 0;

volatile bool inProgress = true;

// Called when ending the app
void endFunction(){
  free(messageToSendToNet);
  free(messageInBuffer);
  free(replyDisplayBuffer);
  network_stop();

  io_close_history_file();

  if(sound_blaster_tts){
    sbtts_read_str(GOODBYE_SND, strlen(GOODBYE_SND), true);
    sbtts_end();
  }

  if(huggingFace){
    printf("Ended DOS Hugging Face client\n");
  } else {
    printf("Ended DOS ChatGPT client\n");
  }
}

// When network received a Break
void networkBreakHandler( ) {
  printf("End\n");
  inProgress = false;
  endFunction();
  exit(1);
}

// Open config file and store the configuration in memory
bool openAndProcessConfigFile(char * filename){
  FILE * configFile;
  #define BUFF_LENGTH 10
  char buff[BUFF_LENGTH];

  configFile = fopen(filename, "r");
  if(configFile == NULL){
    return false;
  }

  fgets(config_apikey, API_KEY_LENGTH_MAX, configFile);

  //Remove trailing newline
  config_apikey[strcspn(config_apikey, "\r\n")] = '\0';

  fgets(config_model, MODEL_LENGTH_MAX, configFile);

  //Remove trailing newline
  config_model[strcspn(config_model, "\r\n")] = '\0';

  fgets(buff, BUFF_LENGTH, configFile);
  config_req_temperature = atof(buff);
  
  fgets(config_proxy_hostname, PROXY_HOST_MAX, configFile);
  //Remove trailing newline
  config_proxy_hostname[strcspn(config_proxy_hostname, "\r\n")] = '\0';

  fgets(buff, BUFF_LENGTH, configFile);
  config_proxy_port = atoi(buff);

  fgets(buff, BUFF_LENGTH, configFile);
  config_outgoing_start_port = strtoul(buff, NULL, 10);

  fgets(buff, BUFF_LENGTH, configFile);
  config_outgoing_end_port = strtoul(buff, NULL, 10);

  fgets(buff, BUFF_LENGTH, configFile);
  config_socketConnectTimeout = strtoul(buff, NULL, 10);

  fgets(buff, BUFF_LENGTH, configFile);
  config_socketResponseTimeout = strtoul(buff, NULL, 10);

  fclose(configFile);
  
  return true;

}

void escapeThisString(char * source, int sourceSize, char * dest, int destMaxSize){

  memset(dest, 0, destMaxSize);

  int destIndex = 0;

  for(int i = 0; i < sourceSize; i++){

    char currentSourceChar = source[i];

    // Need to escape " and backslash
    if(currentSourceChar == '"' || currentSourceChar == '\\'){
      dest[destIndex] = '\\';
      destIndex++;
    }

    dest[destIndex] = currentSourceChar;
    destIndex++;
  }

}

int main(int argc, char * argv[]){
  printf("Started DOS ChatGPT/Hugging Face client %s by Yeo Kheng Meng\n", VERSION);
  printf("Compiled on %s %s\n\n", __DATE__, __TIME__);

  // Process command line arguments -dri and -drr
  for(int i = 0; i < argc; i++){
    char * arg = argv[i];
    if(strstr(arg, "-dri") && strlen(arg) == 4){
      debug_showRequestInfo = true;
    } else if(strstr(arg, "-drr") && strlen(arg) == 4){
      debug_showRawReply = true;
    } else if(strstr(arg, "-drt") && strlen(arg) == 4){
      debug_showTimeStamp = true;
    } else if(strstr(arg, "-cp737") && strlen(arg) == 6){
      codePageInUse = CODE_PAGE_737;
    } else if(strstr(arg, "-f") && strlen(arg) != 2){
      convHistoryGiven = true;

      if((strlen(arg) - 2) > (CONV_HISTORY_PATH_SIZE - 1)){
        printf("History File Path argument exceeded %d characters\n", CONV_HISTORY_PATH_SIZE);
        return -3;
      }
      //Copy after -f
      memcpy(convHistoryPath, arg + 2, strlen(arg) - 2);
    } else if(strstr(arg, "-c") && strlen(arg) != 2){
      configPathGiven = true;

      if((strlen(arg) - 2) > (CONFIG_PATH_SIZE - 1)){
        printf("Config File Path argument exceeded %d characters\n", CONFIG_PATH_SIZE);
        return -4;
      }

      //Copy after -c
      memcpy(configPath, arg + 2, strlen(arg) - 2);
    } else if(strstr(arg, "-hf") && strlen(arg) == 3){
      huggingFace = true;
    } else if(strstr(arg, "-sbtts") && strlen(arg) == 6){
      sound_blaster_tts = true;
    }
  }

  bool configFileOpenStatus;

  if(configPathGiven){
    configFileOpenStatus = openAndProcessConfigFile(configPath);
  } else {
    configFileOpenStatus = openAndProcessConfigFile(CONFIG_FILENAME_DEFAULT);
  }

  if(configFileOpenStatus){

    printf("API/token key contains %d characters\n", strlen(config_apikey));
    printf("Request temperature: %0.1f\n", config_req_temperature);
    printf("Proxy hostname,port: %s:%d\n", config_proxy_hostname, config_proxy_port);
    printf("Outgoing start port: %u, end port: %u\n", config_outgoing_start_port, config_outgoing_end_port);
    printf("Socket connect timeout: %u ms, response timeout: %u ms\n", config_socketConnectTimeout, config_socketResponseTimeout);
    printf("Show request info -dri: %d, raw reply -drr: %d, timestamps -drt: %d\n", debug_showRequestInfo, debug_showRawReply, debug_showTimeStamp);
    printf("Code page -cpXXX: %d\n", codePageInUse);
    printf("Config Path -cX: %s\n", configPathGiven ? configPath : CONFIG_FILENAME_DEFAULT);

    if(convHistoryGiven){
      printf("Conversation history path -fX: %s\n", convHistoryPath);
    } else {
      printf("Conversation history path -fX: Not specified\n");
    }

    if(sound_blaster_tts == false){
      printf("Sound Blaster TTS -sbtts: %d\n", sound_blaster_tts);
    }
        

  } else {
    printf("Cannot open %s config file containing:\nAPI/Token key\nModel\nRequest Temperature\nProxy hostname\nProxy port\nOutgoing start port\nOutgoing end port\nSocket connect timeout (ms)\nSocket response timeout (ms)\n", configPathGiven ? configPath : CONFIG_FILENAME_DEFAULT);
    return -2;
  }

  bool status = network_init(config_outgoing_start_port, config_outgoing_end_port, networkBreakHandler, config_socketConnectTimeout, config_socketResponseTimeout);

  if (status) {
    //printf("Network ok\n");
  } else {
    printf("Cannot init network\n");
    return -1;
  }

  if(convHistoryGiven && !io_open_history_file(convHistoryPath)){
    printf("Cannot open history file to append\n");
    endFunction();
    return -2;
  }

  messageToSendToNet = (char *) calloc (SIZE_MSG_TO_SEND, sizeof(char));
  if(messageToSendToNet == NULL){
    printf("Cannot allocate memory for messageToSendToNet\n");
    network_stop();
    return -1;
  }

  messageInBuffer = (char *) calloc (SIZE_MESSAGE_IN_BUFFER, sizeof(char));
  if(messageInBuffer == NULL){
    printf("Cannot allocate memory for messageInBuffer\n");
    free(messageToSendToNet);
    network_stop();
    return -1;
  }

  replyDisplayBuffer = (char *) calloc (REPLY_DISPLAY_SIZE, sizeof(char));

  if(replyDisplayBuffer == NULL){
    printf("Cannot allocate memory for message Display\n");
    free(messageToSendToNet);
    free(messageInBuffer);
    network_stop();
    return -1;
  }

  if(sound_blaster_tts){
    bool sbtts_init_status = sbtts_init();

    if(sbtts_init_status == false){
      printf("Error: First Byte Text-to-Speech Engine is not installed.\n");
      
      endFunction();
      return -3;
    }
  }

  if(sound_blaster_tts){

    if(huggingFace){
      sbtts_read_str(DOS_HUGGING_FACE_WELCOME_SND, strlen(DOS_HUGGING_FACE_WELCOME_SND), false);
    } else {
      sbtts_read_str(DOS_CHATGPT_WELCOME_SND, strlen(DOS_CHATGPT_WELCOME_SND), false);
    }
  }

  if(huggingFace){
    printf("\n%s (%s). Press ESC to quit.\n", DOS_HUGGING_FACE_WELCOME_MSG, config_model);
  } else {
    printf("\n%s (%s). Press ESC to quit.\n", DOS_CHATGPT_WELCOME_MSG, config_model);
  }



  io_str_newline("Me:");

  int currentMessagePos = 0;

  while(inProgress){

    // Detect if key is pressed
    if ( _bios_keybrd(_KEYBRD_READY) ) {
      char character = _bios_keybrd(_KEYBRD_READ);

      // Detect ESC key for quit
      if(character == 27){
        inProgress = false;
        break;
      }

      // Detect that user has pressed enter
      if(character == '\r'){

        //Don't send empty request
        if(currentMessagePos == 0){
          continue;
        }

        io_write_str_no_print(messageInBuffer, currentMessagePos);

        io_char('\n');
        if(debug_showTimeStamp){
          io_timestamp();
        }

        escapeThisString(messageInBuffer, currentMessagePos, messageToSendToNet, SIZE_MSG_TO_SEND);

        COMPLETION_OUTPUT output;
        memset((void*) &output, 0, sizeof(COMPLETION_OUTPUT));

        if(huggingFace){
          network_get_huggingface_conversation(config_proxy_hostname, config_proxy_port, config_apikey, config_model, messageToSendToNet, config_req_temperature, &output);
        } else {
          network_get_chatgpt_completion(config_proxy_hostname, config_proxy_port, config_apikey, config_model, messageToSendToNet, config_req_temperature, &output);
        }

        if(output.error == COMPLETION_OUTPUT_ERROR_OK){

          if(huggingFace){
            io_str_newline("\nHugging Face:");
          } else {
            io_str_newline("\nChatGPT:");
          }

          memset(replyDisplayBuffer, 0, REPLY_DISPLAY_SIZE);
          replyDisplayPos = 0;

          //To scan for the special format to convert to formatted text
          for(int i = 0; i < output.contentLength; i++){
            char currentChar = output.content[i];
            char nextChar = output.content[i + 1];
            char followingChar = output.content[i + 2];
            
            //Given \n print the newline then advance 2 steps
            if(currentChar == '\\' && nextChar == 'n'){
              replyDisplayBuffer[replyDisplayPos++] = '\n';
              i++;
            // Given " print the " then advance 2 steps
            } else if(currentChar == '\\' && nextChar == '\"'){
              replyDisplayBuffer[replyDisplayPos++] = '\"';
              i++;
            // Given \ print the \ then advance 2 steps
            } else if(currentChar == '\\' && nextChar == '\\'){
              replyDisplayBuffer[replyDisplayPos++] = '\\';
              i++;
            } else {

              CONVERSION_OUTPUT conversionResult;
              conversionResult = utf_to_cp(codePageInUse, currentChar, nextChar, followingChar);

              unsigned char characterToPrint = conversionResult.character;
              int numCharactersToAdvance = conversionResult.charactersUsed - 1;

              replyDisplayBuffer[replyDisplayPos++] = characterToPrint;
              i += numCharactersToAdvance;

            }
          }

          io_str_newline(replyDisplayBuffer);

          if(debug_showRequestInfo){
            io_request_info(output.outPort, output.prompt_tokens, output.completion_tokens);
          }

          if(sound_blaster_tts){
            sbtts_read_str(replyDisplayBuffer, replyDisplayPos, true);
          }

        } else if(output.error == COMPLETION_OUTPUT_ERROR_CHATGPT){
          io_char('\n');
          io_server_error(output.content, output.contentLength);
        } else {
          io_char('\n');
          io_app_error(output.content, output.contentLength);
        }

        if(debug_showRawReply){
          io_str_newline(output.rawData);
        }

        if(debug_showTimeStamp){
          io_timestamp();
        }

        io_str_newline("\nMe:");

        memset(messageInBuffer, 0, SIZE_MESSAGE_IN_BUFFER);
        currentMessagePos = 0;
      } else if((character >= ' ') && (character <= '~')){

        if(currentMessagePos >= SIZE_MESSAGE_IN_BUFFER){
          printf("Reach buffer max\n");
          continue;
        }

        messageInBuffer[currentMessagePos] = character;
        currentMessagePos++;
        printf("%c", character);
        fflush(stdout);

      //Backspace character
      } else if(character == 8){

        if(currentMessagePos > 0){
          // Remove previous character
          printf("%s", "\b \b");
          currentMessagePos--;
          messageInBuffer[currentMessagePos] = '\0';

          fflush(stdout);
        }


      }

    }

    // Call this frequently in the "background" to keep network moving
    network_drivePackets();
  }

  

  endFunction();
  return 0;
}
