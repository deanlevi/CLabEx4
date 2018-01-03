#include <stdio.h>

#include "load_balancer.h"

//int main(int argc, char *argv[]) { // todo check
int main() {
	/*if (argc < 2) { // todo check if needed
		printf("Not enough arguments. Must give expression to grep. Exiting...\n");
		exit(0);
	}*/
	LoadBalancerProperties LoadBalancer;
	InitLoadBalancer(&LoadBalancer);
	CreateSocketsBindAndListen(&LoadBalancer);
	HandleTraffic(&LoadBalancer);
	return SUCCESS_CODE;
}