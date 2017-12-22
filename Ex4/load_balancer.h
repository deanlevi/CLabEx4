#ifndef LOAD_BALANCER_H
#define LOAD_BALANCER_H

#include <WinSock2.h>


#define ERROR_CODE -1
#define SUCCESS_CODE 0
#define MAX_PORT_NUM 64000
#define MIN_PORT_NUM 1024
#define BINDING_SUCCEEDED 0

typedef struct _LoadBalancerProperties {
	SOCKET ServerPortSocket; // todo check SOCKET
	struct sockaddr_in ServerPortService;
	SOCKET HttpPortSocket; // todo check int
	struct sockaddr_in HttpPortService;

} LoadBalancerProperties;

void CreateSockets(LoadBalancerProperties *LoadBalancer);
void ConnectToPortsAndListen(LoadBalancerProperties *LoadBalancer);

#endif