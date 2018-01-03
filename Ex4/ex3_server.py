#!/usr/bin/python2.7 -tt

import socket

NUMBER_OF_SERVERS = 3

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

def ConnectToLoadBalancer(Servers, LoadBalancerPort):
    for Server in range(NUMBER_OF_SERVERS):
        Server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        Server.connect(('127.0.0.1', LoadBalancerPort)) # todo verify
        Servers.append(Server);
    return Servers

def CreateCounterResponse(CurrentActiveServerRequestCounter):
    LengthOfCurrentActiveServerRequestCounter = len(str(CurrentActiveServerRequestCounter))
    ResponseToSendToLoadBalancer = COUNTER_RESPONSE % \
                    (LengthOfCurrentActiveServerRequestCounter, CurrentActiveServerRequestCounter)
    return ResponseToSendToLoadBalancer

def SendResponseToLoadBalancer(ResponseToSend, CurrentActiveServer):
    BytesToSend = len(ResponseToSend)
    TotalSentBytes = 0
    while TotalSentBytes <= BytesToSend:
        CurrentSentBytes = CurrentActiveServer.send(ResponseToSend[TotalSentBytes:])
        TotalSentBytes += CurrentSentBytes

def ReceiveAndSend(Servers):
    ServersRequestCounter = [0, 0, 0]
    CurrentActiveServerIndex = 0
    while(True):
        CurrentRequest = ""
        while "\r\n\r\n" not in CurrentRequest:
            CurrentRequest += CurrentActiveServerIndex.recv(BUFFER_SIZE)
        FirstLineInCurrentRequest = (CurrentRequest.splitlines())[0]
        FirstLineInCurrentRequestAsArrayOfWords = FirstLineInCurrentRequest.split(" ")
        IndexInFirstLine = 0
        while "GET" != FirstLineInCurrentRequestAsArrayOfWords[IndexInFirstLine]:
            IndexInFirstLine += 1;
        GotCounterRequest = FirstLineInCurrentRequestAsArrayOfWords[IndexInFirstLine + 1] == "/counter"
        if GotCounterRequest:
            ServersRequestCounter[CurrentActiveServerIndex] += 1
            ResponseToSend = CreateCounterResponse(ServersRequestCounter[CurrentActiveServerIndex])
        else:
            ResponseToSend = NOT_FOUND_RESPONSE
        SendResponseToLoadBalancer(ResponseToSend, Servers[CurrentActiveServerIndex])
        CurrentActiveServerIndex = (CurrentActiveServerIndex + 1) % 3
    return

def main():
    Servers = []
    LoadBalancerPort = int(sys.argv[1])
    ConnectToLoadBalancer(Servers, LoadBalancerPort)
    ReceiveAndSend(Servers)
    print Servers # todo remove
    print LoadBalancerPort # todo remove
    return

if __name__ == "__main__":
    main()
