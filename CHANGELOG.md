# Change Log
* v0.11 (30 Apr 2023):
* * Support for Hugging Face API
* v0.10 (30 Apr 2023):
* * Can customise path to configuration file
* v0.9 (27 Apr 2023):
* * (New feature) Ability to append conversation history and debug messages to text file
* * (New feature) Display timestamp as a debug option
* * Remove FAR pointers
* * Reduced user entry buffer to 1600 bytes
* * Reduced API body buffer to 12000 bytes
* * Reduced SEND_RECEIVE buffer to 14000 bytes
* v0.8 (9 Apr 2023):
* * Supports Greek [Code Page 737](https://en.wikipedia.org/wiki/Code_page_437) via `-cp737` command line argument.
* * Corrected small bug in Code Page 437 parsing UTF-8 characters starting with 0xE2 that does not return designated unknown character if unknown character is encountered.
* v0.7 (8 Apr 2023):
* * Corrected bug in previous release where previous message/reply memory is not freed after program ends.
* * Now will use one-time malloc allocations of previous message (5000), temp message (5000), GPT reply (8000) buffers to avoid memory fragmentation.
* * Correct memory allocation issue of not using __far when required
* v0.6 (8 Apr 2023):
* * Added new feature to send the previous request and ChatGPT reply to give the model more context in answering the latest request.
* * * Previous request and ChatGPT reply has to be cached
* * * API_BODY_SIZE_BUFFER increased to 15000 bytes
* * Corrected bug in incorrect printing of uint16_t outgoing port value in debug mode
* v0.5 (5 Apr 2023):
* * Corrected bug where the size of bytes to read from MTCP is always the same even though buffer already has some bytes inside from previous read.
* v0.4 (1 Apr 2023):
* * Updated to use MTCP 2023-03-31
* v0.3 (1 Apr 2023):
* * Display characters like accents from [Code Page 437](https://en.wikipedia.org/wiki/Code_page_437)
* * Escape " and \ characters of user input
* * Print \ without escape from JSON
* * Add a 4096 byte buffer for post-escaped message string, API Body buffer increased to 6144 bytes
* * Further wait for 200ms after the last non-zero byte receive to be sure there are no more bytes incoming from the socket
* * Compiled with Open Watcom 2.0 Beta (2023-04-01 build)
* v0.2 (30 Mar 2023):
* * Compiled with Open Watcom 2.0 Beta (2023-03-04 build) that solves the issue of app not starting on some PCs.
* * Show date and time of compilation
* * Will parse and print quotes that were escaped from the JSON reply
* * Reduce size of user text entry buffer from 10240 to 2048 characters to reduce memory usage.
* * Use the same buffer for send and receive on the socket to further cut down memory usage.
* * API Body buffer dropped to 4096 bytes
* * Print only one decimal point for temperature at start
* v0.1 (26 Mar 2023): 
* * Initial release