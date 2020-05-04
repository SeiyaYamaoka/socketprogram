//sc+hostinfo.cpp
//
//Windows Socket TCP Client Mini
#define _CRT_SECURE_NO_WARNINGS

#pragma comment(lib,"wsock32.lib")
#include<Windows.h>
#include<winsock.h>

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define HOST_NAME_SIZE 1024
#define PORT_NO 49152
#define MSG_SIZE 4096

void ErrorProc(SOCKET sock, const char *func_name);
unsigned long gethostinfo(const char *hostname);

int main(void) {
	SOCKET s;					//通信用ソケット
	struct sockaddr_in addr;	//ホストアドレス
	char host[HOST_NAME_SIZE + 1];
	char msg[MSG_SIZE + 1];
	int len;

	WORD mVerReq;
	WSADATA wsadata;
	unsigned long ip_addr;
	mVerReq = MAKEWORD(1, 1);

	if (WSAStartup(mVerReq, &wsadata) != 0) {
		fprintf(stderr, "ソケットライブラリの初期化に失敗しました。\n");
		return 1;
	}

	printf("Server Name : ");
	scanf("%s", host);

	if ((ip_addr = gethostinfo(host)) == 0UL) { ErrorProc(0, "gethostinfo()"); }

	s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET) {
		ErrorProc(0, "socket()");
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ip_addr;
	addr.sin_port = htons(PORT_NO);
	if (connect(s, (struct sockaddr *)&addr, sizeof addr) == SOCKET_ERROR) {
		//エラー処理
		ErrorProc(s, "connect()");
	}
	while (1) {
		len = recv(s, msg, MSG_SIZE, 0);
		if (len == SOCKET_ERROR) {
			ErrorProc(s, "recv()");
		}
		msg[len] = '\0';
		printf("受信: %s\n", msg, 0);
		if (strncmp(msg, "bye", 3) == 0) { break; }
		printf("入力: ");
		scanf("%s", msg);

		len = send(s, msg, (int)strlen(msg), 0);
		if (len == SOCKET_ERROR) {
			ErrorProc(s, "send()");
		}
		printf("送信: %s\n", msg);
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