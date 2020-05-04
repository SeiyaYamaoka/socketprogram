//UDPclient.cpp

//Windows Console UDP socket client sample program
#define _CRT_SECURE_NO_DEPRECATE

#pragma comment(lib,"wsock32.lib")
#include<Windows.h>
#include<winsock.h>
typedef int socklen_t;

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define PORT_NO 49152
#define HOST_SIZE 1024
#define MSG_SIZE 4096

void ErrorProc(SOCKET sock, const char *func_name);
unsigned long gethostinfo(char *host);

int main(void) {
	SOCKET s;
	struct sockaddr_in addr;
	int addr_len;
	char host[HOST_SIZE];
	char msg[MSG_SIZE + 1];
	unsigned long ipaddress;
	int len;
	WORD mVerReq;
	WSADATA wsadata;

	mVerReq = MAKEWORD(1, 1);
	if (WSAStartup(mVerReq, &wsadata) != 0) {
		fprintf(stderr, "Failed in initialization.\n");
		return 1;
	}

	printf("Server Name		: ");
	scanf_s("%s", host,sizeof host);
	ipaddress = gethostinfo(host);
	if (ipaddress == 0UL) {
		fprintf(stderr, "This address is unsolved \n");
		return 1;
	}
	s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s == INVALID_SOCKET)ErrorProc(0, "socket()");
	while (1) {
		printf("Client Input  :");
		scanf_s("%s", msg, sizeof msg);
		memset(&addr, 0, sizeof addr);
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = ipaddress;
		addr.sin_port = htons(PORT_NO);
		len = sendto(s, msg, (int)strlen(msg), 0, (struct sockaddr *)&addr, sizeof addr);
		if (len == SOCKET_ERROR)ErrorProc(s, "sendto()");
		printf("ClientSent    :%s\n", msg);
		addr_len = sizeof addr;
		len = recvfrom(s, msg, MSG_SIZE, 0, (struct sockaddr *)&addr, (socklen_t *)&addr_len);
		if (len == SOCKET_ERROR)ErrorProc(s, "recvfrom()");
		msg[len] = '\0';
		printf("Server Address  : %s\n", inet_ntoa(addr.sin_addr));
		printf("Server Port     : %hu\n", ntohs(addr.sin_port));
		printf("Client Received : %s\n", msg);
		if (strncmp(msg, "bye", 3) == 0)break;
	}
	closesocket(s);
	WSACleanup();
	return 0;

}
void ErrorProc(SOCKET sock, const char *func_name) {
	fprintf(stderr, "Error Code = %d,  ", WSAGetLastError());
	fprintf(stderr, "%s failed.\n", func_name);

	if (sock != 0) {
		closesocket(sock);
	}
	WSACleanup();
	exit(1);
}
unsigned long gethostinfo(char *host) {
	struct hostent *phe;
	unsigned long ip_addr;

	ip_addr = inet_addr(host);
	if (ip_addr != INADDR_NONE) {
		return ip_addr;
	}
	else {
		phe = gethostbyname(host);
		if (phe == NULL)return 0UL;
	}
	return *(unsigned long *)*phe->h_addr_list;
}