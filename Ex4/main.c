#include <stdio.h>

#include "load_balancer.h"

int main() {
	LoadBalancerProperties LoadBalancer;
	InitLoadBalancer(&LoadBalancer);
	CreateSocketsBindAndListen(&LoadBalancer);
	HandleTraffic(&LoadBalancer);
	return SUCCESS_CODE;
}