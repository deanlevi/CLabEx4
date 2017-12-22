#pragma comment(lib,  "ws2_32.lib") // todo check

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include "load_balancer.h"

void CreateSockets(LoadBalancerProperties *LoadBalancer);
int CreateOneSocket();
void ConnectToPortsAndListen(LoadBalancerProperties *LoadBalancer);
void SetSockAddrInAndBind(SOCKET PortSocket, struct sockaddr_in PortService, char *FileNameToUpdatePort);
void GetRandomPortNumber(int *SelectedPort);
void WritePortToFile(int SelectedPort, char *FileNameToUpdatePort);

void CreateSockets(LoadBalancerProperties *LoadBalancer) {
	LoadBalancer->ServerPortSocket = CreateOneSocket();
	LoadBalancer->HttpPortSocket = CreateOneSocket();

	SetSockAddrInAndBind(LoadBalancer->ServerPortSocket, LoadBalancer->ServerPortService, "server_port.txt");
	SetSockAddrInAndBind(LoadBalancer->HttpPortSocket, LoadBalancer->HttpPortService, "http_port.txt"); // todo verify that updates
}

int CreateOneSocket() { // todo fix function
	int ErrorNumber = 0;
	SOCKET NewSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // todo check IPPROTO_TCP
	if (NewSocket == INVALID_SOCKET) {
		ErrorNumber = WSAGetLastError(); // todo check problem with this
		printf("CreateSockets failed to create Socket.\n Error Number is %d", ErrorNumber);
		exit(ERROR_CODE);
	}
	return NewSocket;
}

void ConnectToPortsAndListen(LoadBalancerProperties *LoadBalancer) {
}

void SetSockAddrInAndBind(SOCKET PortSocket, struct sockaddr_in PortService, char *FileNameToUpdatePort) {
	int SelectedPort;
	int BindingReturnValue;
	bool BindingSucceeded = false;

	PortService.sin_family = AF_INET;
	PortService.sin_addr.s_addr = inet_addr("127.0.0.1"); // todo check

	while (!BindingSucceeded) {
		GetRandomPortNumber(&SelectedPort);
		PortService.sin_port = htons(SelectedPort);
		BindingReturnValue = bind(PortSocket, (SOCKADDR*)&PortService, sizeof(PortService));
		if (BindingReturnValue == BINDING_SUCCEEDED) {
			BindingSucceeded = true;
			WritePortToFile(SelectedPort, FileNameToUpdatePort);
		}
	}
}

void GetRandomPortNumber(int *SelectedPort) { // todo check why he said to rand also low ports on purpose
	srand((unsigned)time(NULL));
	*SelectedPort = rand() % (MAX_PORT_NUM - MIN_PORT_NUM);
	*SelectedPort += MIN_PORT_NUM;
}

void WritePortToFile(int SelectedPort, char *FileNameToUpdatePort) {
	FILE *InputFilePtr = NULL;
	InputFilePtr = fopen(FileNameToUpdatePort, "w");
	if (InputFilePtr == NULL) {
		printf("Failed to open file. Exiting...\n");
		exit(ERROR_CODE);
	}
	fprintf(InputFilePtr, "%d", SelectedPort);
	fclose(InputFilePtr);
}