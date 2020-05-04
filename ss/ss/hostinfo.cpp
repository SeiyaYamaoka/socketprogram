//hostinfo.cpp
//
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib,"wsock32.lib")
#include<Windows.h>
#include<winsock.h>
#include<stdio.h>

unsigned long gethostinfo(const char *hostname);

#define HOSTNAME_SIZE 1024

int main(int argc, char **argv) {
	unsigned long ip_addr;
	WORD wVerReq;
	WSADATA wsadata;

	wVerReq = MAKEWORD(1, 1);
	if (WSAStartup(wVerReq, &wsadata) != 0) {
		return 1;
	}
	if (argc <= 1) {
		fprintf(stderr, "Usage: hostinfo.exe hostname hostname...");
		return 1;
	}
	while (--argc) {
		if ((ip_addr = gethostinfo(*++argv)) == 0UL) { return 1; }
		fprintf(stdout, "ip address\t: %s\n", inet_ntoa(*(struct in_addr *)&ip_addr));
	}
	WSACleanup();
	return 0;
}
unsigned long gethostinfo(const char *hostname) {
	struct hostent *phe;
	unsigned long ip_addr;
	int i;
	ip_addr = inet_addr(hostname);
	if (ip_addr == INADDR_NONE) {
		phe = gethostbyname(hostname);
	}
	else {
		phe = gethostbyaddr((char *)&ip_addr, 4, AF_INET);
	}
	if (phe == NULL) {
		return 0UL;
	}
	//----------------------------------
	printf("HostName\t: %s\n", phe->h_name);
	for (i = 0; phe->h_aliases[i] != NULL; i++) {
		printf("Aliase Name[%d]\t: %s\n", i, phe->h_aliases[i]);
	}
	printf("Address Type\t: %hu\n", phe->h_addrtype);
	printf("Address length\t: %hu\n", phe->h_length);
	for (i = 0; phe->h_addr_list[i] != NULL; i++) {
		printf("IP address[%d]\t: %s\n", i, inet_ntoa(*(struct in_addr *)phe->h_addr_list[i]));
	}
	//----------------------------------
	if (*phe->h_addr_list == NULL) {
		return 0UL;
	}
	else {
		return *(unsigned long *)*phe->h_addr_list;
	}
}