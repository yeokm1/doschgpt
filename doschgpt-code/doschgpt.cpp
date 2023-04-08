#include <stdio.h>
#include <stdlib.h>
#include <bios.h>
#include <string.h>

#include "network.h"
#include "utfcp437.h"

#define VERSION "0.7"

// User configuration
#define CONFIG_FILENAME "doschgpt.ini"
#define API_KEY_LENGTH_MAX 200
#define MODEL_LENGTH_MAX 50
#define PROXY_HOST_MAX 100

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

// Message Request
#define SIZE_MSG_TO_SEND 4096
char * messageToSendToNet;

#define SIZE_MESSAGE_IN_BUFFER 2048
char * messageInBuffer;

volatile bool inProgress = true;


// Called when ending the app
void endFunction(){
  free(messageToSendToNet);
  free(messageInBuffer);
  network_stop();

  printf("Ended DOS ChatGPT client\n");
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
  printf("Started DOS ChatGPT client %s by Yeo Kheng Meng\n", VERSION);
  printf("Compiled on %s %s\n\n", __DATE__, __TIME__);

  // Process command line arguments -dri and -drr
  for(int i = 0; i < argc; i++){
    char * arg = argv[i];
    if(strstr(arg, "-dri")){
      debug_showRequestInfo = true;
    } else if(strstr(arg, "-drr")){
      debug_showRawReply = true;
    }
  }

  if(openAndProcessConfigFile(CONFIG_FILENAME)){

    printf("Client config details read from file:\n");
    printf("API key contains %d characters\n", strlen(config_apikey));
    printf("Model: %s\n", config_model);
    printf("Request temperature: %0.1f\n", config_req_temperature);
    printf("Proxy hostname: %s\n", config_proxy_hostname);
    printf("Proxy port: %d\n", config_proxy_port);
    printf("Outgoing start port: %u\n", config_outgoing_start_port);
    printf("Outgoing end port: %u\n", config_outgoing_end_port);
    printf("Socket connect timeout: %u ms\n", config_socketConnectTimeout);
    printf("Socket response timeout: %u ms\n", config_socketResponseTimeout);

    printf("\nDebug request info -dri: %d\n", debug_showRequestInfo);
    printf("Debug raw reply -drr: %d\n", debug_showRawReply);

  } else {
    printf("\nCannot open %s config file containing:\nAPI key\nModel\nRequest Temperature\nProxy hostname\nProxy port\nOutgoing start port\nOutgoing end port\nSocket connect timeout (ms)\nSocket response timeout (ms)\n", CONFIG_FILENAME);
    return -2;
  }

  bool status = network_init(config_outgoing_start_port, config_outgoing_end_port, networkBreakHandler, config_socketConnectTimeout, config_socketResponseTimeout);

  if (status) {
    //printf("Network ok\n");
  } else {
    printf("Cannot init network\n");
    return -1;
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

  printf("\nStart talking to ChatGPT. Press ESC to quit.\n");
  printf("Me:\n");

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

        printf("\n");

        escapeThisString(messageInBuffer, currentMessagePos, messageToSendToNet, SIZE_MSG_TO_SEND);
        COMPLETION_OUTPUT output;
        memset((void*) &output, 0, sizeof(COMPLETION_OUTPUT));
        network_get_completion(config_proxy_hostname, config_proxy_port, config_apikey, config_model, messageToSendToNet, config_req_temperature, &output);

        if(output.error == COMPLETION_OUTPUT_ERROR_OK){
          printf("\nChatGPT:\n");

          //To scan for the special format to convert to formatted text
          for(int i = 0; i < output.contentLength; i++){
            char currentChar = output.content[i];
            char nextChar = output.content[i + 1];
            char followingChar = output.content[i + 2];
            
            //Given \n print the newline then advance 2 steps
            if(currentChar == '\\' && nextChar == 'n'){
              printf("\n");
              i++;
            // Given " print the " then advance 2 steps
            } else if(currentChar == '\\' && nextChar == '\"'){
              printf("\"");
              i++;
            // Given \ print the \ then advance 2 steps
            } else if(currentChar == '\\' && nextChar == '\\'){
              printf("\\");
              i++;
            } else {
              CONVERSION_OUTPUT conversionResult = utf_to_cp437(currentChar, nextChar, followingChar);
              unsigned char characterToPrint = conversionResult.character;
              int numCharactersToAdvance = conversionResult.charactersUsed - 1;

              printf("%c", characterToPrint);
              i += numCharactersToAdvance;

            }
          }
          

          printf("\n");

          if(debug_showRequestInfo){
            printf("Outgoing port %u, %d prompt tokens, %d completion tokens\n", output.outPort, output.prompt_tokens, output.completion_tokens);
          }
          
        } else if(output.error == COMPLETION_OUTPUT_ERROR_CHATGPT){
          printf("\nChatGPT Error:\n%.*s\n", output.contentLength, output.content);
        } else {
          printf("\nApp Error:\n%.*s\n", output.contentLength, output.content);
        }

        if(debug_showRawReply){
          printf("%s\n", output.rawData);
        }
        

        printf("\nMe:\n");

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