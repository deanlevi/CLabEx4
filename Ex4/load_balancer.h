#ifndef LOAD_BALANCER_H
#define LOAD_BALANCER_H

#include <WinSock2.h>


#define ERROR_CODE -1
#define SUCCESS_CODE 0
#define MAX_PORT_NUM 64000
#define MIN_PORT_NUM 1024
#define BINDING_SUCCEEDED 0
#define LISTEN_SUCCEEDED 0
#define NUM_OF_SERVERS 3
#define ACCEPT_FAILED -1
#define BUFFER_SIZE 400 // todo check
#define SEND_RECEIVE_FLAGS 0
//todo check it's ok that all defines are here


typedef struct _LoadBalancerProperties {
	SOCKET ListeningServerPortSocket; // todo check SOCKET
	struct sockaddr_in ListeningServerPortService;
	SOCKET ListeningHttpPortSocket; // todo check int
	struct sockaddr_in ListeningHttpPortService;

	SOCKET HttpPortSocket;
	SOCKET ServerSockets[NUM_OF_SERVERS];

	int CurrentServerSocket;
} LoadBalancerProperties;

void InitLoadBalancer(LoadBalancerProperties *LoadBalancer);
void CreateSocketsBindAndListen(LoadBalancerProperties *LoadBalancer);
void HandleTraffic(LoadBalancerProperties *LoadBalancer);

#endif