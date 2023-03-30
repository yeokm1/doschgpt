package main

import (
	"bufio"
	"fmt"
	"log"
	"net/http"
	"os"
	"strconv"
)

var httpListenPort = 80
var replyText []byte

func handler(responseToRequest http.ResponseWriter, incomingRequest *http.Request) {

	host := incomingRequest.Host
	url := incomingRequest.URL
	log.Printf("Received request for host %s and url %s", host, url)
	log.Printf("%s", url.Path)

	var reply []byte = []byte("Unknown request")

	switch incomingRequest.Method {
	case "POST":
		switch url.Path {
		case "/v1/chat/completions":
			reply = replyText
		}

	}

	// Prepare the requesting socket for writing. Access raw socket by hijacking
	// Reference: https://stackoverflow.com/questions/29531993/accessing-the-underlying-socket-of-a-net-http-response

	hj, ok := responseToRequest.(http.Hijacker)
	if !ok {
		http.Error(responseToRequest, "webserver doesn't support hijacking", http.StatusInternalServerError)
		return
	}
	returnConn, _, err := hj.Hijack()

	if err != nil {
		http.Error(responseToRequest, err.Error(), http.StatusInternalServerError)
		return
	}

	defer returnConn.Close()

	fmt.Println(string(reply))

	returnConn.Write(reply)

	log.Println("End of handler")

}

func main() {

	argsWithoutProg := os.Args[1:]

	if len(argsWithoutProg) >= 3 {
		parsedHTTPPort, err := strconv.ParseInt(argsWithoutProg[0], 10, 32)

		if err != nil {
			log.Printf("Cannot parse argument %s", argsWithoutProg[0])
			return
		}

		httpListenPort = int(parsedHTTPPort)
	}


	replyText, _ = readFile("reply.txt")

	log.Printf("Starting Mock Server listening to %d", httpListenPort)

	http.HandleFunc("/", handler)

	if err := http.ListenAndServe(":"+strconv.Itoa(httpListenPort), nil); err != nil {
		log.Fatal(err)
	}
}

func readFile(filename string) ([]byte, error) {
	file, err := os.Open(filename)

	if err != nil {
		return nil, err
	}
	defer file.Close()

	stats, statsErr := file.Stat()
	if statsErr != nil {
		return nil, statsErr
	}

	var size int64 = stats.Size()
	bytes := make([]byte, size)

	bufr := bufio.NewReader(file)
	_, err = bufr.Read(bytes)

	return bytes, err
}
