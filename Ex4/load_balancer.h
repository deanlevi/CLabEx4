#ifndef LOAD_BALANCER_H
#define LOAD_BALANCER_H

//#include <WinSock2.h> // todo remove
#include <sys/socket.h> // todo verify
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>

#define SUCCESS_CODE 0
#define NUMBER_OF_SERVERS 3

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