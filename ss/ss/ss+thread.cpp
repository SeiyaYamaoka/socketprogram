//ss+thread.cpp
//
//Windows Socket TCP Server Thread版
#define _CRT_SECURE_NO_WARNINGS

#pragma comment(lib,"wsock32.lib")
#include<windows.h>
#include<winsock.h>

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<process.h>

#define PORT_NO 49152
#define MSG_SIZE 4096

void SockThread(void *sock);
void ErrorProc(SOCKET sock, const char *func_name);

int main(void) {
	SOCKET sa;
	SOCKET s;
	struct sockaddr_in addr;
	struct sockaddr_in addrc;
	const char hello[] = "こんにちは。サーバです。「bye」で終わります。";
	char msg[MSG_SIZE + 1];
	int len;
	WORD mVerReq;
	WSADATA wsadata;
	uintptr_t hThread;

	mVerReq = MAKEWORD(1, 1);

	if (WSAStartup(mVerReq, &wsadata) != 0) {
		fprintf(stderr, "Failed in initialization\n");
		return 1;
	}

	sa = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sa == INVALID_SOCKET) {
		ErrorProc(0, "socket()");
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(PORT_NO);
	if (bind(sa, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
		//エラー処理
		ErrorProc(sa, "bind()");
	}

	if (listen(sa, SOMAXCONN) == SOCKET_ERROR) {
		//エラー処理
		ErrorProc(sa, "listen()");
	}

	while (1) {
		len = sizeof addrc;
		s = accept(sa, (struct sockaddr*)&addrc, &len);
		if (s == INVALID_SOCKET) {
			ErrorProc(sa, "accept()");
		}
		printf("Client IP	:%s\n", inet_ntoa(addrc.sin_addr));
		printf("Client Port	:%hu\n", ntohs(addrc.sin_port));

		len = send(s, hello, (int)strlen(hello), 0);

		if (len == SOCKET_ERROR) {
			ErrorProc(s, "send()");
		}
		hThread = _beginthread(SockThread, 0, (void *)s);
		if (hThread == (-1)) {
			printf("thread creation error.\n");
			closesocket(s);
			break;
		}
	}
	closesocket(sa);

	WSACleanup();

	return 0;
}

void SockThread(void *sock) {
	SOCKET s;
	char msg[MSG_SIZE];
	int len;

	s = (SOCKET)sock;
	do {
		len = recv(s, msg, MSG_SIZE, 0);
		if (len == SOCKET_ERROR) {
			ErrorProc(s, "recv()");
		}
		msg[len] = '\0';
		printf("受信: %s\n", msg);
		len = send(s, msg, (int)strlen(msg), 0);
		if (len == SOCKET_ERROR) {
			ErrorProc(s, "send()");
		}
		printf("送信: %s\n", msg);
	} while (strncmp(msg, "bye", 3) != 0);
	closesocket(s);
	_endthread();
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