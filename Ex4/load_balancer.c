#pragma comment(lib,  "ws2_32.lib") // todo check

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

#include "load_balancer.h"

void InitLoadBalancer(LoadBalancerProperties *LoadBalancer);
void CreateSocketsBindAndListen(LoadBalancerProperties *LoadBalancer);
int CreateOneSocket();
void SetSockAddrInAndBind(SOCKET PortSocket, struct sockaddr_in PortService,
						  char *FileNameToUpdatePort, LoadBalancerProperties *LoadBalancer);
void GetRandomPortNumber(int *SelectedPort);
void WritePortToFile(int SelectedPort, char *FileNameToUpdatePort, LoadBalancerProperties *LoadBalancer);
void SetSocketToListen(SOCKET PortSocket, LoadBalancerProperties *LoadBalancer);
void HandleTraffic(LoadBalancerProperties *LoadBalancer);
void HandleAcceptFromHttpPort(LoadBalancerProperties *LoadBalancer);
void HandleAcceptFromServerPort(LoadBalancerProperties *LoadBalancer);
void ReceiveAndSendData(SOCKET SendingSocket, SOCKET ReceivingSocket,
						LoadBalancerProperties *LoadBalancer, bool SendToServerPort);
void CloseSockets(LoadBalancerProperties *LoadBalancer);
void CloseSocketsAndExit(LoadBalancerProperties *LoadBalancer, bool CloseSocketsEnable);

void InitLoadBalancer(LoadBalancerProperties *LoadBalancer) {
	int SocketIndex = 0;
	for (; SocketIndex < NUM_OF_SERVERS; SocketIndex++) {
		LoadBalancer->ServerSockets[SocketIndex] = INVALID_SOCKET;
	}
	LoadBalancer->ListeningServerPortSocket = INVALID_SOCKET;
	LoadBalancer->HttpPortSocket = INVALID_SOCKET;
	LoadBalancer->ListeningHttpPortSocket = INVALID_SOCKET;
	LoadBalancer->CurrentServerSocket = 0;
}

void CreateSocketsBindAndListen(LoadBalancerProperties *LoadBalancer) {
	LoadBalancer->ListeningServerPortSocket = CreateOneSocket();
	if (LoadBalancer->ListeningServerPortSocket == INVALID_SOCKET) {
		CloseSocketsAndExit(LoadBalancer, true);
	}
	LoadBalancer->ListeningHttpPortSocket = CreateOneSocket();
	if (LoadBalancer->ListeningHttpPortSocket == INVALID_SOCKET) {
		CloseSocketsAndExit(LoadBalancer, true);
	}

	SetSockAddrInAndBind(LoadBalancer->ListeningServerPortSocket, LoadBalancer->ListeningServerPortService,
						 "server_port.txt", LoadBalancer);
	SetSockAddrInAndBind(LoadBalancer->ListeningHttpPortSocket, LoadBalancer->ListeningHttpPortService,
						 "http_port.txt", LoadBalancer); // todo verify that updates

	SetSocketToListen(LoadBalancer->ListeningServerPortSocket, LoadBalancer);
	SetSocketToListen(LoadBalancer->ListeningHttpPortSocket, LoadBalancer);
}

int CreateOneSocket() { // todo fix function
	int ErrorNumber = 0;
	SOCKET NewSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // todo check IPPROTO_TCP
	if (NewSocket == INVALID_SOCKET) {
		ErrorNumber = WSAGetLastError(); // todo check problem with this
		printf("CreateOneSocket failed to create Socket.\nError Number is %d\n", ErrorNumber);
	}
	return NewSocket;
}

void SetSockAddrInAndBind(SOCKET PortSocket, struct sockaddr_in PortService,
						  char *FileNameToUpdatePort, LoadBalancerProperties *LoadBalancer) {
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
			WritePortToFile(SelectedPort, FileNameToUpdatePort, LoadBalancer);
		}
	}
}

void GetRandomPortNumber(int *SelectedPort) { // todo check why he said to rand also low ports on purpose
	srand((unsigned)time(NULL));
	*SelectedPort = rand() % (MAX_PORT_NUM - MIN_PORT_NUM);
	*SelectedPort += MIN_PORT_NUM;
}

void WritePortToFile(int SelectedPort, char *FileNameToUpdatePort, LoadBalancerProperties *LoadBalancer) {
	FILE *InputFilePtr = NULL;
	InputFilePtr = fopen(FileNameToUpdatePort, "w");
	if (InputFilePtr == NULL) {
		printf("WritePortToFile failed to open file. Exiting...\n");
		CloseSocketsAndExit(LoadBalancer, true);
	}
	fprintf(InputFilePtr, "%d", SelectedPort);
	fclose(InputFilePtr);
}

void SetSocketToListen(SOCKET PortSocket, LoadBalancerProperties *LoadBalancer) {
	int ListenReturnValue;
	ListenReturnValue = listen(PortSocket, NUM_OF_SERVERS);
	if (ListenReturnValue != LISTEN_SUCCEEDED) {
		int ErrorNumber = WSAGetLastError(); // todo check problem with this
		printf("SetSocketListen failed to set Socket to listen.\nError Number is %d\n", ErrorNumber);
		CloseSocketsAndExit(LoadBalancer, true);
	}
}

void HandleTraffic(LoadBalancerProperties *LoadBalancer) {
	HandleAcceptFromHttpPort(LoadBalancer);
	HandleAcceptFromServerPort(LoadBalancer);
	while (true) {
		ReceiveAndSendData(LoadBalancer->HttpPortSocket, LoadBalancer->ServerSockets[LoadBalancer->CurrentServerSocket],
						   LoadBalancer, true);
		ReceiveAndSendData(LoadBalancer->ServerSockets[LoadBalancer->CurrentServerSocket], LoadBalancer->HttpPortSocket,
						   LoadBalancer, false);
		LoadBalancer->CurrentServerSocket = (LoadBalancer->CurrentServerSocket + 1) % 3;
	}
	CloseSockets(LoadBalancer); // todo check that i never actually reach here
}

void HandleAcceptFromHttpPort(LoadBalancerProperties *LoadBalancer) {
	LoadBalancer->HttpPortSocket = accept(LoadBalancer->ListeningHttpPortSocket,
										  &(LoadBalancer->ListeningHttpPortService), NULL);
	if (LoadBalancer->HttpPortSocket == ACCEPT_FAILED) {
		int ErrorNumber = WSAGetLastError(); // todo check problem with this
		printf("HandleAcceptFromHttpPort failed to accept.\nError Number is %d\n", ErrorNumber);
		CloseSocketsAndExit(LoadBalancer, true);
	}
}

void HandleAcceptFromServerPort(LoadBalancerProperties *LoadBalancer) {
	int SocketIndex = 0;
	for (; SocketIndex < NUM_OF_SERVERS; SocketIndex++) {
		LoadBalancer->ServerSockets[SocketIndex] = accept(LoadBalancer->ListeningServerPortSocket,
														  &(LoadBalancer->ListeningServerPortService), NULL);
		if (LoadBalancer->ServerSockets[SocketIndex] == ACCEPT_FAILED) {
			int ErrorNumber = WSAGetLastError(); // todo check problem with this
			printf("HandleAcceptFromServerPort failed to accept.\nError Number is %d\n", ErrorNumber);
			CloseSocketsAndExit(LoadBalancer, true);
		}
	}
}

void ReceiveAndSendData(SOCKET SendingSocket, SOCKET ReceivingSocket,
						LoadBalancerProperties *LoadBalancer, bool SendToServerPort) {
	int ReceivedBytes = 0;
	int SentBytes = 0;
	int SumOfSentBytes = 0;
	int LeftBytesToSend = 0;
	int EndIndicationsCounter = 0;
	bool FinishedReceiveAndSend = false;
	char ReceivedBuffer[BUFFER_SIZE];
	char *ReceivedBufferSendingPosition = NULL;

	while (!FinishedReceiveAndSend) {
		ReceivedBytes = recv(SendingSocket, ReceivedBuffer, BUFFER_SIZE, SEND_RECEIVE_FLAGS);
		if (ReceivedBytes == SOCKET_ERROR) {
			int ErrorNumber = WSAGetLastError(); // todo check problem with this
			printf("ReceiveAndSendData failed to recv.\nError Number is %d\n", ErrorNumber);
			CloseSocketsAndExit(LoadBalancer, true);
		}
		while (SentBytes < ReceivedBytes) {
			ReceivedBufferSendingPosition = ReceivedBuffer;
			LeftBytesToSend = ReceivedBytes - SumOfSentBytes;
			SentBytes = send(ReceivingSocket, ReceivedBufferSendingPosition, LeftBytesToSend, SEND_RECEIVE_FLAGS);
			if (SentBytes == SOCKET_ERROR) {
				int ErrorNumber = WSAGetLastError(); // todo check problem with this // todo arrange code better
				printf("ReceiveAndSendData failed to send.\nError Number is %d\n", ErrorNumber);
				CloseSocketsAndExit(LoadBalancer, true);
			}
			SumOfSentBytes += SentBytes;
			ReceivedBufferSendingPosition += SentBytes;
		}
		SentBytes = 0;
		SumOfSentBytes = 0;
		UpdateEndIndications(ReceivedBuffer, &EndIndicationsCounter);
		if ((EndIndicationsCounter == 1 && SendToServerPort) || EndIndicationsCounter >= 2) {
			FinishedReceiveAndSend = true;
		}
	}
}

void UpdateEndIndications(char ReceivedBuffer[BUFFER_SIZE], int *EndIndicationsCounter) {
	char *EndIndicationPositionInBuffer = strstr(ReceivedBuffer, "\r\n\r\n");
	if (EndIndicationPositionInBuffer != NULL) {
		*EndIndicationsCounter ++;
		char *RunningOverFirstIndication = "XXXX";
		strncpy(EndIndicationPositionInBuffer, RunningOverFirstIndication, 4);
	}
	EndIndicationPositionInBuffer = strstr(ReceivedBuffer, "\r\n\r\n");
	if (EndIndicationPositionInBuffer != NULL) {
		*EndIndicationsCounter++;
	}
}

void CloseSockets(LoadBalancerProperties *LoadBalancer) {
	int SocketIndex = 0;
	for (; SocketIndex < NUM_OF_SERVERS; SocketIndex++) {
		CloseOneSocket(LoadBalancer->ServerSockets[SocketIndex]);
	}
	CloseOneSocket(LoadBalancer->HttpPortSocket);
	CloseOneSocket(LoadBalancer->ListeningHttpPortSocket);
	CloseOneSocket(LoadBalancer->ListeningServerPortSocket);
}

void CloseOneSocket(SOCKET Socket) {
	int CloseSocketReturnValue;
	if (Socket != INVALID_SOCKET) {
		CloseSocketReturnValue = closesocket(Socket);
		if (CloseSocketReturnValue == SOCKET_ERROR) {
			int ErrorNumber = WSAGetLastError(); // todo check problem with this
			printf("CloseOneSocket failed to close socket.\nError Number is %d\n", ErrorNumber);
			CloseSocketsAndExit(NULL, false);
		}
	}
}

void CloseSocketsAndExit(LoadBalancerProperties *LoadBalancer, bool CloseSocketsEnable) {
	if (CloseSocketsEnable) {
		CloseSockets(LoadBalancer);
	}
	exit(ERROR_CODE);
}