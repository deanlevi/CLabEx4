//#pragma comment(lib,  "ws2_32.lib") // todo check
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

#include "load_balancer.h"

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
void CheckIfGotWholeMessage(char *WholeReceivedBuffer, bool *FinishedReceiveAndSend,
							bool SendToServerPort, int *NumberOfEndIndicationsFound);
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
}

void CreateSocketsBindAndListen(LoadBalancerProperties *LoadBalancer) {
	LoadBalancer->ListeningServerPortSocket = CreateOneSocket(LoadBalancer);
	LoadBalancer->ListeningHttpPortSocket = CreateOneSocket(LoadBalancer);

	printf("CreateSocketsBindAndListen finished creating sockets.\n"); // todo remove
	SetSockAddrInAndBind(LoadBalancer->ListeningServerPortSocket, LoadBalancer->ListeningServerPortService,
						 "server_port", LoadBalancer);
	SetSockAddrInAndBind(LoadBalancer->ListeningHttpPortSocket, LoadBalancer->ListeningHttpPortService,
						 "http_port", LoadBalancer); // todo verify that updates
	printf("CreateSocketsBindAndListen finished binding sockets.\n"); // todo remove
	SetSocketToListen(LoadBalancer->ListeningServerPortSocket, LoadBalancer);
	SetSocketToListen(LoadBalancer->ListeningHttpPortSocket, LoadBalancer);
	printf("CreateSocketsBindAndListen set sockets to listen.\n"); // todo remove
}

int CreateOneSocket(LoadBalancerProperties *LoadBalancer) { // todo fix function
	int NewSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // todo check IPPROTO_TCP
	if (NewSocket == INVALID_SOCKET) {
		//int ErrorNumber = WSAGetLastError(); // todo check problem with this
		printf("CreateOneSocket failed to create Socket.\n");// Error Number is %d\n" , ErrorNumber);
		CloseSocketsAndExit(LoadBalancer, true);
	}
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

void SetSocketToListen(int PortSocket, LoadBalancerProperties *LoadBalancer) {
	int ListenReturnValue;
	ListenReturnValue = listen(PortSocket, NUMBER_OF_SERVERS);
	if (ListenReturnValue != LISTEN_SUCCEEDED) {
		//int ErrorNumber = WSAGetLastError(); // todo check problem with this
		printf("SetSocketToListen failed to set Socket to listen.\n");// Error Number is %d\n", ErrorNumber);
		CloseSocketsAndExit(LoadBalancer, true);
	}
}

void HandleTraffic(LoadBalancerProperties *LoadBalancer) {
	HandleAcceptFromHttpPort(LoadBalancer);
	printf("HandleTraffic - http port accepted.\n"); // todo remove
	HandleAcceptFromServerPort(LoadBalancer);
	printf("HandleTraffic - server port accepted.\n"); // todo remove
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
										  (struct sockaddr*)&(LoadBalancer->ListeningHttpPortService), NULL);
	if (LoadBalancer->HttpPortSocket == ACCEPT_FAILED) {
		//int ErrorNumber = WSAGetLastError(); // todo check problem with this
		printf("HandleAcceptFromHttpPort failed to accept.\n");// Error Number is %d\n", ErrorNumber);
		CloseSocketsAndExit(LoadBalancer, true);
	}
}

void HandleAcceptFromServerPort(LoadBalancerProperties *LoadBalancer) {
	int SocketIndex = 0;
	for (; SocketIndex < NUMBER_OF_SERVERS; SocketIndex++) {
		LoadBalancer->ServerSockets[SocketIndex] = accept(LoadBalancer->ListeningServerPortSocket, // todo check if need to pass NULL, NULL
												 (struct sockaddr*)&(LoadBalancer->ListeningServerPortService), NULL);
		if (LoadBalancer->ServerSockets[SocketIndex] == ACCEPT_FAILED) {
			//int ErrorNumber = WSAGetLastError(); // todo check problem with this
			printf("HandleAcceptFromServerPort failed to accept.\n");// Error Number is %d\n", ErrorNumber);
			CloseSocketsAndExit(LoadBalancer, true);
		}
	}
}

void ReceiveAndSendData(int SendingSocket, int ReceivingSocket,
						LoadBalancerProperties *LoadBalancer, bool SendToServerPort) {
	int ReceivedBytes = 0;
	bool FinishedReceiveAndSend = false;
	char CurrentReceivedBuffer[BUFFER_SIZE];

	char *WholeReceivedBuffer = malloc(sizeof(char) * (BUFFER_SIZE + 1) );
	if (WholeReceivedBuffer == NULL) {
		printf("ReceiveAndSendData failed to allocate memory.\n");
		CloseSocketsAndExit(LoadBalancer, true);
	}
	int SizeOfWholeReceivedBuffer = BUFFER_SIZE;
	int IndexInWholeReceivedBuffers = 0;
	bool NeedToReallocWholeBuffer = false;
	int NumberOfEndIndicationsFound = 0;

	while (!FinishedReceiveAndSend) {
		ReceivedBytes = recv(SendingSocket, CurrentReceivedBuffer, BUFFER_SIZE, SEND_RECEIVE_FLAGS);
		if (ReceivedBytes == SOCKET_ERROR) {
			//int ErrorNumber = WSAGetLastError(); // todo check problem with this
			printf("ReceiveAndSendData failed to recv.\n");// Error Number is %d\n", ErrorNumber);
			CloseSocketsAndExit(LoadBalancer, true);
		}
		printf("ReceiveAndSendData - received chunk of %d bytes. SendToServerPort = %d.\n",
														ReceivedBytes, SendToServerPort); // todo remove
		printf("ReceiveAndSendData - received chunk is:\n%s", CurrentReceivedBuffer); // todo remove
		SendBuffer(ReceivingSocket, ReceivedBytes, CurrentReceivedBuffer, LoadBalancer, SendToServerPort);
		/*ReceivedBufferSendingPosition = CurrentReceivedBuffer;
		while (SumOfSentBytes < ReceivedBytes) {
			LeftBytesToSend = ReceivedBytes - SumOfSentBytes;
			SentBytes = send(ReceivingSocket, ReceivedBufferSendingPosition, LeftBytesToSend, SEND_RECEIVE_FLAGS);
			if (SentBytes == SOCKET_ERROR) {
				//int ErrorNumber = WSAGetLastError(); // todo check problem with this // todo arrange code better
				printf("ReceiveAndSendData failed to send.\n");// Error Number is %d\n", ErrorNumber);
				CloseSocketsAndExit(LoadBalancer, true);
			}
			SumOfSentBytes += SentBytes;
			ReceivedBufferSendingPosition += SentBytes;
		}
		printf("ReceiveAndSendData - sent chunk of %d bytes. SendToServerPort = %d.\n",
														SumOfSentBytes, SendToServerPort); // todo remove
		SumOfSentBytes = 0;
		*/
		NeedToReallocWholeBuffer = IndexInWholeReceivedBuffers + ReceivedBytes > SizeOfWholeReceivedBuffer;
		if (NeedToReallocWholeBuffer) {
			SizeOfWholeReceivedBuffer += BUFFER_SIZE;
			WholeReceivedBuffer = (char*) realloc(WholeReceivedBuffer, SizeOfWholeReceivedBuffer);
			if (WholeReceivedBuffer == NULL) {
				printf("ReceiveAndSendData failed to reallocate memory.\n");
				CloseSocketsAndExit(LoadBalancer, true);
			}
			printf("ReceiveAndSendData - reallocated WholeReceivedBuffer to %d. SendToServerPort = %d.\n",
														SizeOfWholeReceivedBuffer, SendToServerPort); // todo remove
		}
		IndexInWholeReceivedBuffers += ReceivedBytes;
		strcat(WholeReceivedBuffer, CurrentReceivedBuffer);
		printf("ReceiveAndSendData - WholeReceivedBuffer is:\n%s", WholeReceivedBuffer); // todo remove

		CheckIfGotWholeMessage(WholeReceivedBuffer, &FinishedReceiveAndSend, SendToServerPort, &NumberOfEndIndicationsFound);
	}
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
			//int ErrorNumber = WSAGetLastError(); // todo check problem with this // todo arrange code better
			printf("ReceiveAndSendData failed to send.\n");// Error Number is %d\n", ErrorNumber);
			CloseSocketsAndExit(LoadBalancer, true);
		}
		SumOfSentBytes += SentBytes;
		ReceivedBufferSendingPosition += SentBytes;
	}
	printf("ReceiveAndSendData - sent chunk of %d bytes. SendToServerPort = %d.\n",
													SumOfSentBytes, SendToServerPort); // todo remove
}

void CheckIfGotWholeMessage(char *WholeReceivedBuffer, bool *FinishedReceiveAndSend,
							bool SendToServerPort, int *NumberOfEndIndicationsFound) {
	char *EndIndicationPositionInBuffer = strstr(WholeReceivedBuffer, "\r\n\r\n");
	bool FoundEndIndication = EndIndicationPositionInBuffer != NULL;
	
	if (!FoundEndIndication) {
		return;
	}
	(*NumberOfEndIndicationsFound)++;
	if (SendToServerPort || *NumberOfEndIndicationsFound == 2) {
		*FinishedReceiveAndSend = true;
		return;
	}

	char *RunningOverFirstIndication = "XXXX";
	strncpy(EndIndicationPositionInBuffer, RunningOverFirstIndication, 4);
	EndIndicationPositionInBuffer = strstr(WholeReceivedBuffer, "\r\n\r\n");
	FoundEndIndication = EndIndicationPositionInBuffer != NULL;

	if (FoundEndIndication) {
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
			//int ErrorNumber = WSAGetLastError(); // todo check problem with this
			printf("CloseOneSocket failed to close socket.\n");// Error Number is %d\n", ErrorNumber);
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