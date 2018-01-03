#ifndef LOAD_BALANCER_H
#define LOAD_BALANCER_H

//#include <WinSock2.h> // todo remove
#include <sys/socket.h> // todo verify
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define ERROR_CODE -1
#define SOCKET_ERROR -1
#define INVALID_SOCKET (int) (~0)
#define SUCCESS_CODE 0
#define MAX_PORT_NUM 64000
#define MIN_PORT_NUM 1024
#define SERVER_ADDRESS_STR "127.0.0.1"
#define BINDING_SUCCEEDED 0
#define LISTEN_SUCCEEDED 0
#define NUMBER_OF_SERVERS 3
#define ACCEPT_FAILED -1
#define BUFFER_SIZE 100 // todo check
#define SEND_RECEIVE_FLAGS 0
//todo check it's ok that all defines are here


typedef struct _LoadBalancerProperties {
	int ListeningServerPortSocket; // todo check SOCKET
	struct sockaddr_in ListeningServerPortService;
	int ListeningHttpPortSocket; // todo check int
	struct sockaddr_in ListeningHttpPortService;

	int HttpPortSocket;
	int ServerSockets[NUMBER_OF_SERVERS];

	int CurrentServerSocket;
} LoadBalancerProperties;

void InitLoadBalancer(LoadBalancerProperties *LoadBalancer);
void CreateSocketsBindAndListen(LoadBalancerProperties *LoadBalancer);
void HandleTraffic(LoadBalancerProperties *LoadBalancer);

#endif