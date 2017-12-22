#include <stdio.h>

#include "load_balancer.h"

int main(int argc, char *argv[]) {
	/*if (argc < 2) { // todo check if needed
		printf("Not enough arguments. Must give expression to grep. Exiting...\n");
		exit(0);
	}*/
	LoadBalancerProperties LoadBalancer;
	CreateSocketsBindAndListen(&LoadBalancer);
	//ConnectToPortsAndListen(&LoadBalancer);
	return SUCCESS_CODE;
}