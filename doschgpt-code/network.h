#include <TYPES.H>


#define COMPLETION_OUTPUT_ERROR_OK 0
#define COMPLETION_OUTPUT_ERROR_CHATGPT 1
#define COMPLETION_OUTPUT_ERROR_APP 2


// Structure to return after calling network_get_completion
typedef struct
{
    // One the COMPLETION_OUTPUT_ERROR_X
    int error;

    // Tokens parsed from JSON
    int completion_tokens;
    int prompt_tokens;

    // The parsed text comepletion
    char * content;

    // Length of text completion
    int contentLength;

    // Outgoing port used in the current request
    uint16_t outPort;

    // Raw Header and JSON reply for debug use
    char * rawData;
    
} COMPLETION_OUTPUT;


//Callback for network_init() on Break
typedef void (*EndCallback)(void);

// Init MTCP network stack and setup other variables
bool network_init(uint16_t startPort, uint16_t endPort, EndCallback endCall, uint16_t socketConnectTimeout, uint16_t socketResponseTimeout);

// Stop MTCP network before shutting down
void network_stop();

// Open a socket to the proxy server. Socket number is stored internally
// hostname: Hostname or IP address. MTCP will resolve the hostname to IP
// port: Port number of proxy server to connect to
// outgoingPort: outgoing port to use
bool network_connectToSocket(char * hostname, int port, uint16_t * outgoingPort);

// Close currently open socket
void network_closeCurrentSocket();

// Call this regularly to process packets in the background
void network_drivePackets();

// Send a request and return the reply. Internally calls network_connectToSocket()
// hostname: hostname of proxy
// port: Proxy port
// to_send: Raw data to send to server
// to_send_size: Size of to_send
// to_receive: Receive buffer
// to_receive_size: Size of to_receive
// outgoingPort: outgoing port to use
bool network_send_receive(char * hostname, int port, char * to_send, int to_send_size, char * to_receive, int to_receive_size, uint16_t * outgoingPort);

// Forms and makes API call to chat completion. Internally calls network_send_receive()
// hostname: hostname of proxy
// port: Proxy port
// api_key: API key
// model: GPT model (See OpenAL dev page)
// message: Message request to send (See OpenAL dev page)
// temperature: randomness of reply (See OpenAL dev page)
// output: struct containing parsed json output
bool network_get_completion(char * hostname, int port, char * api_key, char * model, char * message, float temperature, COMPLETION_OUTPUT * output);