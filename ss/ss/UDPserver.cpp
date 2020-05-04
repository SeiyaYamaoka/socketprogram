//UDPserver.cpp

//Windows Console UDP socket server sample program
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

int main(void) {
	SOCKET s;
	struct sockaddr_in addr;
	int addr_len;
	char msg[MSG_SIZE + 1];
	int recv_len;
	int send_len;
	WORD mVerReq;
	WSADATA wsadata;

	mVerReq = MAKEWORD(1, 1);
	if (WSAStartup(mVerReq, &wsadata) != 0) {
		fprintf(stderr, "Failed in initialization.\n");
		return 1;
	}

	s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s == INVALID_SOCKET)ErrorProc(s, "socket");

	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(PORT_NO);
	if (bind(s, (struct sockaddr *)&addr, sizeof addr) == SOCKET_ERROR)ErrorProc(s, "bind()");

	while (1) {
		addr_len = sizeof addr;
		recv_len = recvfrom(s, msg, MSG_SIZE, 0, (struct sockaddr *)&addr, (socklen_t *)&addr_len);
		if (recv_len == SOCKET_ERROR)ErrorProc(s, "recvfrom");
		msg[recv_len] = '\0';

		printf("Client Address   :%s\n", inet_ntoa(addr.sin_addr));
		printf("Client Port      :%hu\n", ntohs(addr.sin_port));
		printf("Server Received  :%s\n", msg);

		send_len = sendto(s, msg, (int)strlen(msg), 0, (struct sockaddr *)&addr, sizeof addr);
		if (send_len == SOCKET_ERROR)ErrorProc(s, "sendto");
		printf("Server Sent      :%s\n", msg);
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
