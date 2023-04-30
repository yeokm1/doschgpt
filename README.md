# doschgpt

A proof-of-concept ChatGPT and Hugging Face client for DOS. 

<img src="images\doschgpt-front-combined.jpg" width="1000">

Photos of the client running on [my 1984 IBM 5155 Portable PC](https://github.com/yeokm1/retro-configs/tree/master/desktops/ibm-5155) with a 4.77Mhz Intel 8088 CPU with MS-DOS 6.22.

As there are no native HTTPS APIs for DOS, a [HTTP-to-HTTPS proxy](https://github.com/yeokm1/http-to-https-proxy) like this I've written running on a modern machine is needed.

This program is heavily based on sample code in the DOS networking [MTCP library](http://brutmanlabs.org/mTCP/). The program also requires a [DOS Packet Driver](https://en.wikipedia.org/wiki/PC/TCP_Packet_Driver) to be loaded and MTCP to be set for the machine/VM.

**This program was written in a short time as a toy project. It has not been vigorously tested thus is NOT meant for "production" use.**

## Using the application

Application binary can be found in the `releases` directory or Github Releases section but do the following first.

1. Both OpenAI and Hugging Face requires an API key/token to use their APIs. Follow the [instructions on OpenAP ](https://platform.openai.com/account/api-keys) or [Hugging Face](https://huggingface.co/settings/tokens) websites to obtain this key before proceeding.

2. Download and start up [http-to-https-proxy](https://github.com/yeokm1/http-to-https-proxy/releases) on a modern machine/SBC.

3. The application requires a config file. By default it will use the filename `doschgpt.ini` but you can specify another path with the `-c` argument. Modify the configuration file to suit your needs in this order. A sample file can be found with the binary.

* API key: Place your key without quotes (API key in this sample file has been revoked)
* Model: Language model to use, can use `gpt-3.5-turbo` for ChatGPT. Hugging Face can use `facebook/blenderbot-400M-distill` or `microsoft/DialoGPT-large`.
* Request Temperature: How random the completion will be. More [OpenAI details](https://platform.openai.com/docs/guides/chat/instructing-chat-models) and [Hugging Face details](https://huggingface.co/docs/api-inference/detailed_parameters#conversational-task)
* Proxy hostname: Hostname IP of the proxy
* Proxy port: Proxy Port
* Outgoing start port: Start of a range of randomly selected outgoing port
* Outgoing end port: End of a range of randomly selected outgoing port
* Socket connect timeout (ms): How long to wait when attempting to connect to proxy
* Socket response timeout (ms): How long to wait for OpenAI's servers to reply

4. Ensure that your DOS environment has loaded the following:

* Packet Driver
* MTCP Config Environment variable `MTCPCFG`
* MTCP Config file configured by DHCP

5. Just launch `doschgpt.exe` in your machine and fire away. Press the ESC key to quit the application. You may use the following optional command line arguments.

* `-hf`: To use Hugging Face instead of ChatGPT
* `-cdoschgpt.ini`: Replace `doschgpt.ini` with any other config filepath you desire. There is no space between the `-c` and the filepath.
* `-dri`: Print the outgoing port, number of prompt and completion tokens used after each request. Tokens are only provided by ChatGPT.
* `-drr`: Display the raw server return headers and json reply
* `-drt`: Display the timestamp of the latest request/reply
* `-cp737`: Supports Greek [Code Page 737](https://en.wikipedia.org/wiki/Code_page_737). Ensure code page is loaded before starting the program.
* `-fhistory.txt`: Append conversation history to new/existing text file. File will also include debug messages if specified. Replace `history.txt` with any other filepath you desire. There is no space between the `-f` and the filepath.

Example usage:

```bash
# Connects to ChatGPT using config file at doschgpt.ini
doschgpt.exe

# Connects to Hugging Face using config file at hf.ini with timestamps after each request and reply
doschgpt.exe -hf -chf.ini -drt
```

<img src="images\doschgpt-5155-front-start.jpg" width="500">

Parsed options will be displayed.

## Compilation

To compile this application, you have to use Open Watcom 2.0 beta which you can download from [here](https://github.com/open-watcom/open-watcom-v2/releases/tag/2023-04-01-Build). Open Watcom 2.0 for 64-bit Windows which was released on 2023-04-01 02:52:44 is used. The v1.9 version seems to create binaries with issues on some platforms.

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
curl https://api-inference.huggingface.co/models/facebook/blenderbot-400M-distill -X POST -H "Authorization: Bearer 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" -d '{"inputs": {"text":"What is retro-computing?"}, "parameters": { "temperature": 1.0 } }'
# Hugging Face API with history
curl https://api-inference.huggingface.co/models/facebook/blenderbot-400M-distill -X POST -H "Authorization: Bearer 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" -d '{"inputs": {"past_user_inputs": ["What is retrocomputing?"], "generated_responses": ["A retro computer is a type of computer that was invented in the 1950s."], "text": "I want one", "parameters": { "temperature": 1.0 } }'
```

# Changelog

Go to [CHANGELOG.md](CHANGELOG.md).