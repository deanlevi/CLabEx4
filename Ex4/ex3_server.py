#!/usr/bin/python2.7

import sys
import socket

BUFFER_SIZE = 100

COUNTER_RESPONSE = "HTTP/1.0 200 OK\r\nContent-Type: " + \
                   "text/html\r\nContent-Length: " + \
                   "%d\r\n\r\n%d\r\n\r\n"

NOT_FOUND_RESPONSE = "HTTP/1.1 404 Not Found\r\n" + \
                     "Content-type: text/html\r\n" + \
                     "Content-length: 113\r\n" + \
                     "<html><head><title>Not Found</title></head><body>\r\n" + \
                     "Sorry, the object you requested was not found.\r\n" + \
                     "</body></html>\r\n"

def ConnectToLoadBalancer(LoadBalancerPort):
    Server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    Server.connect(('127.0.0.1', LoadBalancerPort)) # todo verify
    print "python - connected to server" # todo remove
    return Server

def CreateCounterResponse(ServerRequestCounter):
    LengthOfServerRequestCounter = len(str(ServerRequestCounter))
    ResponseToSendToLoadBalancer = COUNTER_RESPONSE % \
                    (LengthOfServerRequestCounter, ServerRequestCounter)
    return ResponseToSendToLoadBalancer

def SendResponseToLoadBalancer(ResponseToSend, Server):
    BytesToSend = len(ResponseToSend)
    TotalSentBytes = 0
    while TotalSentBytes <= BytesToSend:
        CurrentSentBytes = Server.send(ResponseToSend[TotalSentBytes:])
        TotalSentBytes += CurrentSentBytes

def ReceiveAndSend(Servers):
    ServerRequestCounter = 0
    while(True):
        CurrentRequest = ""
        while "\r\n\r\n" not in CurrentRequest:
            CurrentRequest += Server.recv(BUFFER_SIZE)
        FirstLineInCurrentRequest = (CurrentRequest.splitlines())[0]
        FirstLineInCurrentRequestAsArrayOfWords = FirstLineInCurrentRequest.split(" ")
        IndexInFirstLine = 0
        while "GET" != FirstLineInCurrentRequestAsArrayOfWords[IndexInFirstLine]:
            IndexInFirstLine += 1;
        GotCounterRequest = FirstLineInCurrentRequestAsArrayOfWords[IndexInFirstLine + 1] == "/counter"
        if GotCounterRequest:
            ServerRequestCounter += 1
            ResponseToSend = CreateCounterResponse(ServerRequestCounter)
        else:
            ResponseToSend = NOT_FOUND_RESPONSE
        SendResponseToLoadBalancer(ResponseToSend, Server)
    return

def main():
    LoadBalancerPort = int(sys.argv[1])
    Server = ConnectToLoadBalancer(LoadBalancerPort)
    ReceiveAndSend(Server)
    return

if __name__ == "__main__":
    main()
