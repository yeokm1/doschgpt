#include "network.h"

#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "utils.h"
#include "packet.h"
#include "arp.h"
#include "dns.h"
#include "tcp.h"
#include "tcpsockm.h"
#include "timer.h"

#define API_CHAT_COMPLETION "POST /v1/chat/completions HTTP/1.1\r\nContent-Type: application/json\r\nAuthorization: Bearer %s\r\nHost: api.openai.com\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s"
#define API_BODY "{ \"model\": \"%s\", \"messages\": [{\"role\": \"user\", \"content\": \"%s\"}], \"temperature\": %.1f }"

#define API_BODY_SIZE_BUFFER 4096
#define SEND_RECEIVE_BUFFER 16384

char * api_body_buffer = NULL;
char * sendRecvBuffer = NULL;

//Network configuration obtained from network_init()
uint16_t startingPort;
uint16_t endingPort;
uint16_t network_socketConnectTimeout;
uint16_t network_socketResponseTimeout;

// Only one socket is used in this app
TcpSocket *mySocket = NULL;

// Check this flag once in a while to see if the user wants out.
volatile uint8_t CtrlBreakDetected = 0;

EndCallback endF;

void __interrupt __far ctrlBreakHandler( ) {
    CtrlBreakDetected = 1;
    endF();
}

void __interrupt __far ctrlCHandler( ) {
  // Do Nothing - Ctrl-C is a legal character
}

bool network_init(uint16_t startPort, uint16_t endPort, EndCallback endCall, uint16_t socketConnectTimeout, uint16_t socketResponseTimeout){

    // Setup mTCP environment
    if(Utils::parseEnv()){
        fprintf(stderr, "Cannot parse mtcp env\n" );
        return false;
    }

    // Initialize TCP/IP stack with 1 socket
    if(Utils::initStack(1, TCP_SOCKET_RING_SIZE, ctrlBreakHandler, ctrlCHandler)){
        fprintf(stderr, "Cannot init stack\n" );
        return false;
    }

    sendRecvBuffer = (char *) calloc(SEND_RECEIVE_BUFFER, sizeof(char));

    if(sendRecvBuffer == NULL){
        printf("Cannot allocate memory for sendRecvBuffer\n");

        network_stop();

        return false;
    }


    api_body_buffer = (char *) calloc(API_BODY_SIZE_BUFFER, sizeof(char));

    if(api_body_buffer == NULL){
        printf("Cannot allocate memory for API Body Buffer\n");
        
        // Free previous buffer
        free(sendRecvBuffer);

        network_stop();

        return false;
    }

    startingPort = startPort;
    endingPort = endPort;

    endF = endCall;
    network_socketConnectTimeout = socketConnectTimeout;
    network_socketResponseTimeout = socketResponseTimeout;
    
    return true;
}

void network_stop(){

    if(sendRecvBuffer != NULL){
        free(sendRecvBuffer);
        sendRecvBuffer = NULL;
    }

    if(api_body_buffer != NULL){
        free(api_body_buffer);
        api_body_buffer = NULL;
    }

    network_closeCurrentSocket();
    
    Utils::endStack( );
    //Utils::dumpStats(stderr);
}

bool network_connectToSocket(char * hostname, int port, uint16_t * outgoingPort){

    network_closeCurrentSocket();

    IpAddr_t serverAddr;

    // Resolve the name and send the request
    int8_t rc2 = Dns::resolve(hostname, serverAddr, 1 );
    if ( rc2 < 0 ) {
      fprintf( stderr, "Error resolving server\n" );
      return false;
    }

    uint8_t done = 0;

    while ( !done ) {

      if ( CtrlBreakDetected ) break;
      if ( !Dns::isQueryPending( ) ) break;

        network_drivePackets();

    }

    //fprintf(stderr, "Server resolved to %d.%d.%d.%d - connecting\n\n", serverAddr[0], serverAddr[1], serverAddr[2], serverAddr[3] );

    mySocket = TcpSocketMgr::getSocket();

    mySocket->setRecvBuffer(SEND_RECEIVE_BUFFER);

    uint16_t currentPort = ((uint16_t) rand()) % (endingPort + 1 - startingPort) + startingPort;
    *outgoingPort = currentPort;

    // Non-blocking connect.  Wait 10 seconds before giving up.
    int8_t rc = mySocket->connect(currentPort++, serverAddr, port, network_socketConnectTimeout);

    if(currentPort > endingPort){
        currentPort = startingPort;
    }

    if (rc != 0 ) {
        //fprintf(stderr, "Socket open failed %d\n", rc);
        return false;
    }

    return true;
}

void network_closeCurrentSocket(){
    if(mySocket != NULL){
        mySocket->close();
        TcpSocketMgr::freeSocket(mySocket);
        mySocket = NULL;
    }
}

void network_drivePackets(){
    PACKET_PROCESS_SINGLE;
    Arp::driveArp();
    Tcp::drivePackets();
}

bool network_send_receive(char * hostname, int port, char * to_send, int to_send_size, char * to_receive, int to_receive_size, uint16_t * outgoingPort){
    bool status = network_connectToSocket(hostname, port, outgoingPort);
    if (status) {
        //fprintf(stderr, "Can connect\n");
    } else {
        //fprintf(stderr, "Cannot connect\n");
        return false;
    }

    int16_t bytesSent = mySocket->send((unsigned char *) to_send, to_send_size);
    memset(to_receive, 0, to_receive_size);

    if(bytesSent == to_send_size){
        //fprintf(stderr, "Waiting for data\n");
        int bytesReceivedThisInstant = 0;

        clockTicks_t startTime = TIMER_GET_CURRENT();

        bool receivedfirstByte = false;

        char * pointerToReceiveAt = to_receive;
        int bytesReceivedSoFar = 0;

        while(!mySocket->isClosed()){

            int bytesAbleToReceive = to_receive_size - bytesReceivedSoFar;
            
            //Receive as much as we can then loop again as sometimes cannot fill up the buffer on first try
            bytesReceivedThisInstant = mySocket->recv((unsigned char *) pointerToReceiveAt, bytesAbleToReceive);
            pointerToReceiveAt = pointerToReceiveAt + bytesReceivedThisInstant;

            if(bytesReceivedThisInstant > 0){
                receivedfirstByte = true;
            } else {
                // We no longer get any bytes after receiving something. Menas end of message
                if(receivedfirstByte){
                    break;
                }
            }

            network_drivePackets();

            // Timeout after no reply for some time
            if(Timer_diff(startTime, TIMER_GET_CURRENT()) > TIMER_MS_TO_TICKS(network_socketResponseTimeout) ) {
                status = false;
                break;
            }
        }
        
    } else {
        fprintf(stderr, "Did not send required %d bytes\n", to_send_size);
        status = false;
    }

    network_closeCurrentSocket();
    return status;
}

bool network_get_completion(char * hostname, int port, char * api_key, char * model, char * message, float temperature, COMPLETION_OUTPUT * output){
    
    memset(api_body_buffer, 0, API_BODY_SIZE_BUFFER);
    int actual_body_size = snprintf(api_body_buffer, API_BODY_SIZE_BUFFER, API_BODY, model, message, temperature);

    memset(sendRecvBuffer, 0, SEND_RECEIVE_BUFFER);
    snprintf(sendRecvBuffer, SEND_RECEIVE_BUFFER, API_CHAT_COMPLETION, api_key, actual_body_size, api_body_buffer);
    //puts(requestBuffer);

    int actualRequestBufferLength = strlen(sendRecvBuffer);
    bool status = network_send_receive(hostname, port, sendRecvBuffer, actualRequestBufferLength, sendRecvBuffer, SEND_RECEIVE_BUFFER, &output->outPort);

    output->error = COMPLETION_OUTPUT_ERROR_OK;
    output->rawData = sendRecvBuffer;

    if(status){
        //puts(recvBuffer);

        //Find if contains error
        char * errorPtr = strstr(sendRecvBuffer, "\"error\": {");

        if(errorPtr){
            //fprintf(stderr, "Found error\n");
            char * errorMsgKeyPtr = strstr(sendRecvBuffer, "\"message\": \"");

            if(errorMsgKeyPtr != NULL){

                //Advance to start of message
                char * messageStartPointer = errorMsgKeyPtr + 12;

                //Locate message termination
                char * messageEndPointer = strstr(messageStartPointer, "\",");

                if(messageEndPointer != NULL){
                    int length = messageEndPointer - messageStartPointer;
                    output->error = COMPLETION_OUTPUT_ERROR_CHATGPT;
                    output->content = messageStartPointer;
                    output->contentLength = length;
                } else {
                    output->error = COMPLETION_OUTPUT_ERROR_APP;
                    output->content = "Error but no error_message ending found";
                    output->contentLength = strlen(output->content);
                }
            } else {
                output->error = COMPLETION_OUTPUT_ERROR_APP;
                output->content = "Error but no error_message ending found";
                output->contentLength = strlen(output->content);
            }

        } else {
            //Search for prompt_tokens key
            char * prompt_token_ptr = strstr(sendRecvBuffer, "\"prompt_tokens\":");

            if(prompt_token_ptr){
                //Jump to number position
                output->prompt_tokens = strtol(prompt_token_ptr + 16, NULL, 10);
            }

            char * completion_token_ptr = strstr(sendRecvBuffer, "\"completion_tokens\":");

            if(completion_token_ptr){
                //Jump to number position
                output->completion_tokens = strtol(completion_token_ptr + 20, NULL, 10);
            }

            char * content_ptr = strstr(sendRecvBuffer, "\"content\":");

            if(content_ptr){
                //Advance to start of content
                char * contentStartPointer = content_ptr + 11;

                //Locate message termination
                char * contentEndPointer = strstr(contentStartPointer, "\"}");

                if(contentEndPointer != NULL){
                    output->content = contentStartPointer;
                    output->contentLength = contentEndPointer - contentStartPointer;
                } else {
                    output->error = COMPLETION_OUTPUT_ERROR_APP;
                    output->content = "Cannot find end of content";
                    output->contentLength = strlen(output->content);
                }
            } else {
                output->error = COMPLETION_OUTPUT_ERROR_APP;
                output->content = "Cannot find content";
                output->contentLength = strlen(output->content);
            }
        }
    } else {
        output->error = COMPLETION_OUTPUT_ERROR_APP;
        output->content = "Cannot connect to socket or response timeout";
        output->contentLength = strlen(output->content);
    }

    // if(output->error == 1){
    //     puts(recvBuffer);
    // }
    
    return status;
}