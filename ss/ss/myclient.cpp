//sc.cpp
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

int main(void) {
	SOCKET s;					//通信用ソケット
	struct sockaddr_in addr;	//ホストアドレス
	char host[HOST_NAME_SIZE + 1];
	char msg[MSG_SIZE + 1];
	int len;

	WORD mVerReq;
	WSADATA wsadata;
	mVerReq = MAKEWORD(1, 1);

	if (WSAStartup(mVerReq, &wsadata) != 0) {
		fprintf(stderr, "ソケットライブラリの初期化に失敗しました。\n");
		return 1;
	}

	printf("Server IP Address : ");
	scanf("%s", host);

	s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET) {
		ErrorProc(0, "socket()");
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(host);
	addr.sin_port = htons(PORT_NO);
	if (connect(s, (struct sockaddr *)&addr, sizeof addr) == SOCKET_ERROR) {
		//エラー処理
		ErrorProc(s, "connect()");
	}

	fd_set read_fds;//構造体を用意


	while (1) {

		FD_ZERO(&read_fds);//初期化
		FD_SET(s, &read_fds);//調べたいソケットをすべて登録

		struct timeval timeout;
		timeout.tv_sec = 5;//1秒でタイムアウト
		timeout.tv_usec = 0;
		int count;
		count = select(FD_SETSIZE, &read_fds, 0, 0, &timeout);//countに着信の総数が入る。


		if (count > 0) {//受信したとき
			if (FD_ISSET(s, &read_fds)) {//新しく来たやつ
				len = recv(s, msg, MSG_SIZE, 0);
				if (len == SOCKET_ERROR) {
					ErrorProc(s, "recv()");
				}
				msg[len] = '\0';
				printf("受信: %s\n", msg, 0);
				if (strncmp(msg, "bye", 3) == 0) { break; }
			}
		}

	
		//printf("入力: ");
		//scanf("%s", msg);
		//len = send(s, msg, (int)strlen(msg), 0);
		//if (len == SOCKET_ERROR) {
		//	ErrorProc(s, "send()");
		//}
		//printf("送信: %s\n", msg);
		//
		//printf("roopおわり\n");
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