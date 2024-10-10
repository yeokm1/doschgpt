# doschgpt

A proof-of-concept ChatGPT, Hugging Face and Ollama client for DOS. 

<img src="images\doschgpt-front-combined.jpg" width="1000">

Photos of the client running on [my 1984 IBM 5155 Portable PC](https://github.com/yeokm1/retro-configs/tree/master/desktops/ibm-5155) with a 4.77Mhz Intel 8088 CPU with MS-DOS 6.22.

As there are no native HTTPS APIs for DOS, a [HTTP-to-HTTPS proxy](https://github.com/yeokm1/http-to-https-proxy) like this I've written running on a modern machine is needed. Proxy is not required for Ollama servers as they just use unencrypted HTTP.

This program is heavily based on sample code in the DOS networking [MTCP library](http://brutmanlabs.org/mTCP/). The program also requires a [DOS Packet Driver](https://en.wikipedia.org/wiki/PC/TCP_Packet_Driver) to be loaded and MTCP to be set for the machine/VM.

Using the Text-to-speech feature requires a Sound Blaster-compatible card and 8Mhz CPU.

To reduce coding complexity and memory usage, the app only sends the previous request/reply set together with the current request to provide the conversational context to the model instead of the entire conversation history. Do notify me if you believe this is an issue for you and you prefer more history to be sent.

**This program was written in a short time as a toy project.** It has not been vigorously tested thus is **NOT** meant for "production" use.

## Using the application

A [setup video "How to install the MS-DOS ChatGPT Client"](https://www.youtube.com/watch?v=-hLCr38IYyM) has been posted by [retrorygaming](https://www.youtube.com/@retrorygaming). You can use this as a reference setup tutorial.

Application binary can be found in the `releases` directory or Github Releases section but do the following first.

1. Both OpenAI and Hugging Face requires an API key/token to use their APIs. Follow the [instructions on OpenAI](https://platform.openai.com/account/api-keys) or [Hugging Face](https://huggingface.co/settings/tokens) websites to obtain this key before proceeding. A `read` token for Hugging Face is sufficient.

2. Download and start up [http-to-https-proxy](https://github.com/yeokm1/http-to-https-proxy/releases) on a modern machine/SBC. Proxy is not required for Ollama but you need to set the `OLLAMA_HOST` environment variable in your server to `0.0.0.0` allow external access.

3. The application requires a config file. By default it will use the filename `doschgpt.ini` but you can specify another path with the `-c` argument. Modify the configuration file to suit your needs in this order. A sample file can be found with the binary.

* API key: Place your key without quotes (API key in this sample file has been revoked). For Ollama, leave a dummy line here.
* Model: Language model to use such as but not limited to. `gpt-4o` for ChatGPT. `mistralai/Mistral-7B-Instruct-v0.3` for Hugging Face. More details and models for Hugging Face can be found [here](https://huggingface.co/tasks/conversational). `llama3.2` for Ollama.
* Request Temperature: How random the completion will be. More [OpenAI details](https://platform.openai.com/docs/guides/chat/instructing-chat-models) and [Hugging Face details](https://huggingface.co/docs/api-inference/detailed_parameters#conversational-task)
* Proxy hostname: Hostname IP of the proxy or Ollama server
* Proxy port: Proxy port or Ollama port.
* Outgoing start port: Start of a range of randomly selected outgoing port
* Outgoing end port: End of a range of randomly selected outgoing port
* Socket connect timeout (ms): How long to wait when attempting to connect to proxy
* Socket response timeout (ms): How long to wait for OpenAI's servers to reply

4. Ensure that your DOS environment has loaded the following:

* Packet Driver
* MTCP Config Environment variable `MTCPCFG`
* MTCP Config file configured by DHCP or Static IP
* Text-to-speech feature requires the `BLASTER` variable such as `SET BLASTER=A220 I5 D1 T4` to be set.

5. Just launch `doschgpt.exe` in your machine and fire away. Press the ESC key to quit the application. You may use the following optional command line arguments.

* `-hf`: To use Hugging Face instead of ChatGPT
* `-ol`: To use Ollama instead of ChatGPT
* `-cdoschgpt.ini`: Replace `doschgpt.ini` with any other config filepath you desire. There is no space between the `-c` and the filepath.
* `-dri`: Print the outgoing port, number of prompt and completion tokens used after each request. Tokens are only provided by ChatGPT and Ollama.
* `-drr`: Display the raw server return headers and json reply
* `-drt`: Display the timestamp of the latest request/reply
* `-cp737`: Supports Greek [Code Page 737](https://en.wikipedia.org/wiki/Code_page_737). Ensure code page is loaded before starting the program.
* `-fhistory.txt`: Append conversation history to new/existing text file. File will also include debug messages if specified above. Replace `history.txt` with any other filepath you desire. There is no space between the `-f` and the filepath.
* `-sbtts`: Able to read server reply using a text-to-speech driver used by Dr. Sbaitso.

Example usage:

```bash
# Connects to ChatGPT using config file at doschgpt.ini
doschgpt.exe

# Connects to Hugging Face using config file at hf.ini with timestamps after each request and reply
doschgpt.exe -hf -chf.ini -drt

# Connects to Ollama using config file at ollama.ini and use Sound Blaster text to speech engine
doschgpt.exe -sbtts -ol -collama.ini
```

<img src="images\doschgpt-5155-front-start.jpg" width="500">

Parsed options will be displayed.

### Release details

* doschgpt.exe: Main binary
* doschgpt.ini: Sample configuration file for ChatGPT
* hf.ini: Sample configuration file for Hugging Face
* ollama.ini: Sample configuration file for Ollama

* These files are from the release of Dr Sbaitso and have to be placed in the same directory as `doschgpt.exe`.
    * Sbtalker.exe: Smoothtalker by First Byte text-to-speech engine that loads as a TSR (This is called by the client on start)
    * Blaster.drv: Used by Smoothtalker to talk to a Sound Blaster card
    * Remove.exe: Unloads the Smoothtalker TSR (This is called by the client on end)
    * Read.exe: Reads its command line arguments to the Smoothtalker TSR (No longer used with integration of [dosbtalk](https://github.com/systoolz/dosbtalk))

## Compilation

To compile this application, you have to use Open Watcom 2.0 beta which you can download from [here](https://github.com/open-watcom/open-watcom-v2/releases/tag/2024-10-03-Build). Open Watcom 2.0 for 64-bit Windows which was released on 2024-10-03 03:16:39 UTC is used. The v1.9 version seems to create binaries with issues on some platforms.

During installation, Open Watcom may prompt to install the environment variables. I chose to not do that to avoid having those variables being permanent. Instead I use a batch file to set the variables whenever I need to compile.

The program is compiled via a Makefile that is referenced from MTCP.

```bash
# Open cmd.exe
cd doschgpt-code

# If using Open Watcom v2.0 beta installed to C:\WATCOM2
20setenv.bat

# If using Open Watcom v1.9 installed to C:\WATCOM (Not recommended)
19setenv.bat

# To compile
wmake

#  Only if using Open Watcom 1.9. To patch the Open Watcom runtime to support Compaq Portable. Not needed for Open Watcom 2.0 beta.
PTACH.exe doschgpt.exe doschgpt.map -ml

# To clean
wmake clean
```

This application compiles against the [MTCP library](http://brutmanlabs.org/mTCP/). I have unzipped the latest version [mTCP-src_2023-03-31.zip](http://www.brutman.com/mTCP/download/mTCP-src_2023-03-31.zip) at the time of development to the `mtcpsrc` directory. When Brutman updates this library again in future, simply replace the contents of the `mtcpsrc` directory with the new library.

`PTACH.exe` is a Win NT program compiled from MTCP sources.

## Development

I use Visual Studio Code text editor to code for ease of use. For ease of testing, I used a virtual machine to run the application as 16-bit DOS applications and the MTCP network stack cannot run on modern Windows.

<img src="images\doschgpt-vbox.png" width="600">

More details of my setup can be found [here](https://github.com/yeokm1/retro-configs/tree/master/vms/vbox-dos622).

To easily transfer the binary, I used Python to host my build directory as a temporary webserver. Then use the MTCP tool `htget` to fetch the binary.

```bash
# On modern machine with binary
python3 -m http.server 8000

# Run on DOS machine/VM
htget -o doschgpt.exe http://X.X.X.X:8000/doschgpt.exe
```

### Mock proxy

[OpenAI implements rate limits on their API](https://platform.openai.com/docs/guides/rate-limits/overview) hence we should minimise calling their API repeatedly.

To avoid calling the OpenAI's servers during testing, we can mock the server's using this `mockprox.go`Go program that will replay the contents of `reply.txt` whenever the API call is received. 

<img src="images\doschgpt-mock-proxy.png" width="600">

```bash
cp correct.txt reply.txt
go build mockprox.go
mockprox.exe
```

## APIs

```bash
# Test API directly
curl https://api.openai.com/v1/chat/completions -H "Content-Type: application/json" -H "Authorization: Bearer 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" -d '{ "model": "gpt-3.5-turbo", "messages": [{"role": "user", "content": "What is MS-DOS?"}], "temperature": 0.7 }'

# Call through the https proxy for testing
curl --proxy "http://192.168.1.144:8080" https://api.openai.com/v1/chat/completions -H "Content-Type: application/json" -H "Authorization: Bearer sk-EhmTsEsKyH4qHZL2mr3hT3BlbkFJd6AcfdBrujJsBBGPRcQ" -d '{ "model": "gpt-3.5-turbo", "messages": [{"role": "user", "content": "What is MS-DOS?"}], "temperature": 0.7 }'

# Hugging Face API
curl -X POST -H "Authorization: Bearer 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" -H "Content-Type: application/json" -d '{"inputs": "[INST]Tell me about Singapore[/INST]", "parameters": { "temperature": 0.7 , "max_new_tokens": 400} }' https://api-inference.huggingface.co/models/mistralai/Mistral-7B-Instruct-v0.3

# Hugging Face API with history
curl -X POST -H "Authorization: Bearerhf_cLcKjUgeSUDjtdnYQrxLvErXNkAVubAZDO" -H "Content-Type: application/json" -d '{"inputs": "[INST]Tell me about Singapore in 5 words[/INST]Vibrant, Multicultural, Skyscrapers, Gardens, Clean.[INST]Where is it?[/INST]", "parameters": { "temperature": 0.7 , "max_new_tokens": 400} }' https://api-inference.huggingface.co/models/mistralai/Mistral-7B-Instruct-v0.3
```

# Changelog

Go to [CHANGELOG.md](CHANGELOG.md).

# Third party sources

1. [MTCP TCP stack by Michael Brutman](http://brutmanlabs.org/mTCP/)
2. [Text to speech from SmoothTalker by First Byte and Creative Text-to-Speech Reader downloaded from Winworld](https://winworldpc.com/product/dr-sbaitso/2x)
3. [Dosbtalk libary to interface with First-Byte Engine directly](https://github.com/systoolz/dosbtalk)