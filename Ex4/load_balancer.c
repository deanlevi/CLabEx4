//#pragma comment(lib,  "ws2_32.lib") // todo check
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "load_balancer.h"

#define ERROR_CODE -1
#define SOCKET_ERROR -1
#define INVALID_SOCKET (int) (~0)
#define MAX_PORT_NUM 64000
#define MIN_PORT_NUM 1024
#define SERVER_ADDRESS_STR "127.0.0.1"
#define BINDING_SUCCEEDED 0
#define LISTEN_SUCCEEDED 0
#define ACCEPT_FAILED -1
#define BUFFER_SIZE 200
#define SEND_RECEIVE_FLAGS 0
#define COMMUNICATION_FINISHED 0

void InitLoadBalancer(LoadBalancerProperties *LoadBalancer);
void CreateSocketsBindAndListen(LoadBalancerProperties *LoadBalancer);
int CreateOneSocket(LoadBalancerProperties *LoadBalancer);
void SetSockAddrInAndBind(int PortSocket, struct sockaddr_in PortService,
						  char *FileNameToUpdatePort, LoadBalancerProperties *LoadBalancer);
void GetRandomPortNumber(int *SelectedPort);
void WritePortToFile(int SelectedPort, char *FileNameToUpdatePort, LoadBalancerProperties *LoadBalancer);
void SetSocketToListen(int PortSocket, LoadBalancerProperties *LoadBalancer);
void HandleTraffic(LoadBalancerProperties *LoadBalancer);
void HandleAcceptFromHttpPort(LoadBalancerProperties *LoadBalancer);
void HandleAcceptFromServerPort(LoadBalancerProperties *LoadBalancer);
void ReceiveAndSendData(int SendingSocket, int ReceivingSocket,
						LoadBalancerProperties *LoadBalancer, bool SendToServerPort);
void SendBuffer(int ReceivingSocket, int BytesToSend, char *CurrentReceivedBuffer,
				LoadBalancerProperties *LoadBalancer, bool SendToServerPort) ;
void CheckIfGotWholeMessage(char *WholeReceivedBuffer, bool *FinishedReceiveAndSend, bool SendToServerPort);
void CloseSockets(LoadBalancerProperties *LoadBalancer);
void CloseOneSocket(int Socket);
void CloseSocketsAndExit(LoadBalancerProperties *LoadBalancer, bool CloseSocketsEnable);

void InitLoadBalancer(LoadBalancerProperties *LoadBalancer) {
	int SocketIndex = 0;
	for (; SocketIndex < NUMBER_OF_SERVERS; SocketIndex++) {
		LoadBalancer->ServerSockets[SocketIndex] = INVALID_SOCKET;
	}
	LoadBalancer->ListeningServerPortSocket = INVALID_SOCKET;
	LoadBalancer->HttpPortSocket = INVALID_SOCKET;
	LoadBalancer->ListeningHttpPortSocket = INVALID_SOCKET;
	LoadBalancer->CurrentServerSocket = 0;
	srand((unsigned)time(NULL));
}

void CreateSocketsBindAndListen(LoadBalancerProperties *LoadBalancer) {
	LoadBalancer->ListeningServerPortSocket = CreateOneSocket(LoadBalancer);
	LoadBalancer->ListeningHttpPortSocket = CreateOneSocket(LoadBalancer);

	printf("CreateSocketsBindAndListen finished creating sockets.\n"); // todo remove
	SetSockAddrInAndBind(LoadBalancer->ListeningHttpPortSocket, LoadBalancer->ListeningHttpPortService,
						 "http_port", LoadBalancer); // todo verify that updates
	SetSocketToListen(LoadBalancer->ListeningHttpPortSocket, LoadBalancer);
	SetSockAddrInAndBind(LoadBalancer->ListeningServerPortSocket, LoadBalancer->ListeningServerPortService,
						 "server_port", LoadBalancer);
	SetSocketToListen(LoadBalancer->ListeningServerPortSocket, LoadBalancer);
	printf("CreateSocketsBindAndListen finished binding sockets.\n"); // todo remove
	printf("CreateSocketsBindAndListen set sockets to listen.\n"); // todo remove
}

int CreateOneSocket(LoadBalancerProperties *LoadBalancer) { // todo fix function
	int NewSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // todo check IPPROTO_TCP
	if (NewSocket == INVALID_SOCKET) {
		printf("CreateOneSocket failed to create Socket.\nError Number is %s\n" , strerror(errno));
		CloseSocketsAndExit(LoadBalancer, true);
	}
	/*int VarToSendToSockOpt = 1; // todo remove
	if (setsockopt(NewSocket, SOL_SOCKET, SO_REUSEADDR, &VarToSendToSockOpt, sizeof(int)) < 0) {
		printf("CreateOneSocket failed to setsockopt(SO_REUSEADDR).\n");// Error Number is %d\n" , ErrorNumber);
		CloseSocketsAndExit(LoadBalancer, true);
	}*/
	return NewSocket;
}

void SetSockAddrInAndBind(int PortSocket, struct sockaddr_in PortService,
						  char *FileNameToUpdatePort, LoadBalancerProperties *LoadBalancer) {
	int SelectedPort;
	int BindingReturnValue;
	bool BindingSucceeded = false;

	PortService.sin_family = AF_INET;
	PortService.sin_addr.s_addr = inet_addr(SERVER_ADDRESS_STR); // todo check

	while (!BindingSucceeded) {
		GetRandomPortNumber(&SelectedPort);
		PortService.sin_port = htons(SelectedPort);
		BindingReturnValue = bind(PortSocket, (struct sockaddr*)&PortService, sizeof(PortService));
		if (BindingReturnValue == BINDING_SUCCEEDED) {
			BindingSucceeded = true;
			printf("SetSockAddrInAndBind %s is bound to port %d.\n", FileNameToUpdatePort, SelectedPort); // todo remove
			WritePortToFile(SelectedPort, FileNameToUpdatePort, LoadBalancer);
		}
	}
}

void GetRandomPortNumber(int *SelectedPort) { // todo check why he said to rand also low ports on purpose
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

void SetSocketToListen(int PortSocket, LoadBalancerProperties *LoadBalancer) {
	int ListenReturnValue;
	ListenReturnValue = listen(PortSocket, NUMBER_OF_SERVERS);
	if (ListenReturnValue != LISTEN_SUCCEEDED) {
		printf("SetSocketToListen failed to set Socket to listen.\nError Number is %s\n", strerror(errno));
		CloseSocketsAndExit(LoadBalancer, true);
	}
}

void HandleTraffic(LoadBalancerProperties *LoadBalancer) {
	HandleAcceptFromHttpPort(LoadBalancer);
	printf("HandleTraffic - http port accepted.\n"); // todo remove
	HandleAcceptFromServerPort(LoadBalancer);
	printf("HandleTraffic - server port accepted.\n"); // todo remove
	while (true) {
		printf("HandleTraffic - active server index is %d.\n", LoadBalancer->CurrentServerSocket); // todo remove
		ReceiveAndSendData(LoadBalancer->HttpPortSocket, LoadBalancer->ServerSockets[LoadBalancer->CurrentServerSocket],
						   LoadBalancer, true);
		printf("HandleTraffic - finished receiving data from http port and sending it to server.\n"); // todo remove
		ReceiveAndSendData(LoadBalancer->ServerSockets[LoadBalancer->CurrentServerSocket], LoadBalancer->HttpPortSocket,
						   LoadBalancer, false);
		printf("HandleTraffic - finished receiving data from server port and sending it to http.\n"); // todo remove
		LoadBalancer->CurrentServerSocket = (LoadBalancer->CurrentServerSocket + 1) % 3;
		CloseOneSocket(LoadBalancer->HttpPortSocket);
		HandleAcceptFromHttpPort(LoadBalancer);
		printf("HandleTraffic - http port accepted.\n"); // todo remove
	}
	CloseSockets(LoadBalancer); // todo check its ok that i never actually reach here
}

void HandleAcceptFromHttpPort(LoadBalancerProperties *LoadBalancer) {
	socklen_t HttpPortServiceAddrLen = sizeof(LoadBalancer->ListeningHttpPortService);
	LoadBalancer->HttpPortSocket = accept(LoadBalancer->ListeningHttpPortSocket,
										 (struct sockaddr*)&(LoadBalancer->ListeningHttpPortService),
										 &HttpPortServiceAddrLen);
	if (LoadBalancer->HttpPortSocket == ACCEPT_FAILED) {
		printf("HandleAcceptFromHttpPort failed to accept.\nError Number is %s\n", strerror(errno));
		CloseSocketsAndExit(LoadBalancer, true);
	}
}

void HandleAcceptFromServerPort(LoadBalancerProperties *LoadBalancer) {
	int SocketIndex = 0;
	socklen_t ServerPortServiceAddrLen = sizeof(LoadBalancer->ListeningServerPortService);
	for (; SocketIndex < NUMBER_OF_SERVERS; SocketIndex++) {
		LoadBalancer->ServerSockets[SocketIndex] = accept(LoadBalancer->ListeningServerPortSocket,
												  (struct sockaddr*)&(LoadBalancer->ListeningServerPortService),
												  &ServerPortServiceAddrLen);
		if (LoadBalancer->ServerSockets[SocketIndex] == ACCEPT_FAILED) {
			printf("HandleAcceptFromServerPort failed to accept.\nError Number is %s\n", strerror(errno));
			CloseSocketsAndExit(LoadBalancer, true);
		}
	}
}

void ReceiveAndSendData(int SendingSocket, int ReceivingSocket,
						LoadBalancerProperties *LoadBalancer, bool SendToServerPort) {
	int ReceivedBytes = 0;
	bool FinishedReceiveAndSend = false;
	char CurrentReceivedBuffer[BUFFER_SIZE];

	char *WholeReceivedBuffer = (char *) malloc(sizeof(char) * (BUFFER_SIZE + 1));
	if (WholeReceivedBuffer == NULL) {
		printf("ReceiveAndSendData failed to allocate memory.\n");
		CloseSocketsAndExit(LoadBalancer, true);
	}
	int SizeOfWholeReceivedBuffer = BUFFER_SIZE + 1;
	int IndexInWholeReceivedBuffers = 0;
	bool NeedToReallocWholeBuffer = false;
	WholeReceivedBuffer[IndexInWholeReceivedBuffers] = '\0';

	while (!FinishedReceiveAndSend) {
		ReceivedBytes = recv(SendingSocket, CurrentReceivedBuffer, BUFFER_SIZE, SEND_RECEIVE_FLAGS);
		if (ReceivedBytes == SOCKET_ERROR) {
			printf("ReceiveAndSendData failed to recv.\nError Number is %s\n", strerror(errno));
			CloseSocketsAndExit(LoadBalancer, true);
		}
		else if (ReceivedBytes == COMMUNICATION_FINISHED) {
			printf("ReceiveAndSendData communication finished\n"); // todo remove
			FinishedReceiveAndSend = true;
		}
		printf("ReceiveAndSendData - received chunk of %d bytes. SendToServerPort = %d.\n",
														ReceivedBytes, SendToServerPort); // todo remove
		//printf("ReceiveAndSendData - received chunk is:\n%s", CurrentReceivedBuffer); // todo remove
		SendBuffer(ReceivingSocket, ReceivedBytes, CurrentReceivedBuffer, LoadBalancer, SendToServerPort);
		NeedToReallocWholeBuffer = IndexInWholeReceivedBuffers + ReceivedBytes >= SizeOfWholeReceivedBuffer;
		if (NeedToReallocWholeBuffer) {
			SizeOfWholeReceivedBuffer += BUFFER_SIZE;
			WholeReceivedBuffer = (char*) realloc(WholeReceivedBuffer, sizeof(char) * (SizeOfWholeReceivedBuffer));
			if (WholeReceivedBuffer == NULL) {
				printf("ReceiveAndSendData failed to reallocate memory.\n");
				free(WholeReceivedBuffer);
				CloseSocketsAndExit(LoadBalancer, true);
			}
			printf("ReceiveAndSendData - reallocated WholeReceivedBuffer to %d. SendToServerPort = %d.\n",
														SizeOfWholeReceivedBuffer, SendToServerPort); // todo remove
		}
		IndexInWholeReceivedBuffers += ReceivedBytes;
		strncat(WholeReceivedBuffer, CurrentReceivedBuffer, ReceivedBytes);
		//printf("ReceiveAndSendData - WholeReceivedBuffer is:\n%s", WholeReceivedBuffer); // todo remove

		CheckIfGotWholeMessage(WholeReceivedBuffer, &FinishedReceiveAndSend, SendToServerPort);
	}
	WholeReceivedBuffer[IndexInWholeReceivedBuffers] = '\0';
	//printf("ReceiveAndSendData - WholeReceivedBuffer is:\n%s", WholeReceivedBuffer); // todo remove
	printf("\n\nReceiveAndSendData - finished\n\n"); // todo remove
	free(WholeReceivedBuffer);
}

void SendBuffer(int ReceivingSocket, int BytesToSend, char *CurrentReceivedBuffer,
				LoadBalancerProperties *LoadBalancer, bool SendToServerPort) {
	int SumOfSentBytes = 0;
	int LeftBytesToSend = BytesToSend;
	int SentBytes = 0;
	char *ReceivedBufferSendingPosition = CurrentReceivedBuffer;
	while (SumOfSentBytes < BytesToSend) {
		LeftBytesToSend = BytesToSend - SumOfSentBytes;
		SentBytes = send(ReceivingSocket, ReceivedBufferSendingPosition, LeftBytesToSend, SEND_RECEIVE_FLAGS);
		if (SentBytes == SOCKET_ERROR) {
			printf("ReceiveAndSendData failed to send.\nError Number is %s\n", strerror(errno));
			CloseSocketsAndExit(LoadBalancer, true);
		}
		SumOfSentBytes += SentBytes;
		ReceivedBufferSendingPosition += SentBytes;
	}
	printf("ReceiveAndSendData - sent chunk of %d bytes. SendToServerPort = %d.\n",
													SumOfSentBytes, SendToServerPort); // todo remove
}

void CheckIfGotWholeMessage(char *WholeReceivedBuffer, bool *FinishedReceiveAndSend, bool SendToServerPort) {
	char *FirstEndIndicationPositionInBuffer = strstr(WholeReceivedBuffer, "\r\n\r\n");
	bool FoundEndIndication = FirstEndIndicationPositionInBuffer != NULL;
	
	if (!FoundEndIndication) {
		return;
	}
	printf("CheckIfGotWholeMessage found first match.\n"); // todo remove
	if (SendToServerPort) {
		*FinishedReceiveAndSend = true;
		return;
	}

	char *SecondEndIndicationPositionInBuffer = strstr(FirstEndIndicationPositionInBuffer + 4, "\r\n\r\n");
	FoundEndIndication = SecondEndIndicationPositionInBuffer != NULL;

	if (FoundEndIndication) {
		printf("CheckIfGotWholeMessage found second match.\n"); // todo remove
		*FinishedReceiveAndSend = true;
		return;
	}
}

void CloseSockets(LoadBalancerProperties *LoadBalancer) {
	int SocketIndex = 0;
	for (; SocketIndex < NUMBER_OF_SERVERS; SocketIndex++) {
		CloseOneSocket(LoadBalancer->ServerSockets[SocketIndex]);
	}
	CloseOneSocket(LoadBalancer->HttpPortSocket);
	CloseOneSocket(LoadBalancer->ListeningHttpPortSocket);
	CloseOneSocket(LoadBalancer->ListeningServerPortSocket);
}

void CloseOneSocket(int Socket) {
	int CloseSocketReturnValue;
	if (Socket != INVALID_SOCKET) {
		//CloseSocketReturnValue = closesocket(Socket);
		CloseSocketReturnValue = close(Socket); // todo check close
		if (CloseSocketReturnValue == SOCKET_ERROR) {
			printf("CloseOneSocket failed to close socket.\nError Number is %s\n", strerror(errno));
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