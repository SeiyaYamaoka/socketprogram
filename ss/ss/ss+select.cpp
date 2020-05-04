//ss.cpp
//
//Windows Socket TCP Server Mini
#define _CRT_SECURE_NO_WARNINGS

#pragma comment(lib,"wsock32.lib")
#include<windows.h>
#include<winsock.h>

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define PORT_NO 49152
#define MSG_SIZE 4096

#define MAXSOC 3

void ErrorProc(SOCKET sock, const char *func_name);

int main(void) {
	SOCKET sa;//��t�p
	//SOCKET s;

	struct sockaddr_in addr;
	struct sockaddr_in addrc;
	const char hello[] = "����ɂ��́B�T�[�o�ł��B�ubye�v�ŏI���܂��B";
	const char bye[] = "bye";
	char msg[MSG_SIZE + 1];
	int len;
	WORD mVerReq;
	WSADATA wsadata;

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
		//�G���[����
		ErrorProc(sa, "bind()");
	}


	if (listen(sa, SOMAXCONN) == SOCKET_ERROR) {
		//�G���[����
		ErrorProc(sa, "listen()");
	}

	fd_set read_fds;//�\���̂�p��

	struct socinf{
		SOCKET socket;
		bool flg;
	};

	
	socinf soc[MAXSOC]; //������M�p

	//������
	for (int i = 0; i < MAXSOC; i++) {
		soc[i].flg = false;
	}

	SOCKET s1;
	//printf("sa: %d\n", sa);
	//printf("s1: %d\n", s1);
	int time = 10;
	int timecnt = 0;

	//bool s1flg = false;
	while (1) {

		FD_ZERO(&read_fds);//������
		FD_SET(sa, &read_fds);//���ׂ����\�P�b�g�����ׂēo�^

		//if(s1flg){
		//	FD_SET(s1, &read_fds);//���ׂ����\�P�b�g�����ׂēo�^
		//}

		for (int i = 0; i < MAXSOC; i++) {
			if (soc[i].flg) {
				FD_SET(soc[i].socket, &read_fds);
			}
		}

		struct timeval timeout;
		timeout.tv_sec = 5;//1�b�Ń^�C���A�E�g
		timeout.tv_usec = 0;
		int count;
		count = select(FD_SETSIZE, &read_fds, 0, 0, &timeout);//count�ɒ��M�̑���������B


		if (count > 0) {//��M�����Ƃ�
			//printf("count %d\n", count);

			len = sizeof addrc;

			if (FD_ISSET(sa, &read_fds)) {//�V�����������
				//printf("sa: %d\n",sa);
				int val;
				int max=0;
				for (int i = 0; i < MAXSOC; i++) {
					if (soc[i].flg==false) {
						val = i;
						break;
					}
					max = i;
				}
				if (max == MAXSOC-1) {
					s1 = accept(sa, (struct sockaddr*)&addrc, &len);
					if (s1 == INVALID_SOCKET) {
						ErrorProc(s1, "accept()");
					}
					printf("s1: %d\n",s1);
					printf("Client IP	:%s\n", inet_ntoa(addrc.sin_addr));
					printf("Client Port	:%hu\n", ntohs(addrc.sin_port));
					printf("�\�P�b�g�̍ő吔�𒴂����̂�bye�𑗐M���܂����B\n");
					len = send(s1, bye, (int)strlen(bye), 0);//�ő�\�P�b�g���𒴂����Ƃ���bye�𑗐M����
					if (len == SOCKET_ERROR) {
						ErrorProc(s1, "send()");
					}
					
				}
				else {
					soc[val].socket = accept(sa, (struct sockaddr*)&addrc, &len);
					if (soc[val].socket == INVALID_SOCKET) {
						ErrorProc(soc[val].socket, "accept()");
					}
					printf("soc[%d].socket: %d\n", val, soc[val].socket);
					printf("Client IP	:%s\n", inet_ntoa(addrc.sin_addr));
					printf("Client Port	:%hu\n", ntohs(addrc.sin_port));

					len = send(soc[val].socket, hello, (int)strlen(hello), 0);//����ɂ��̓T�[�o�[�ł��𑗐M
					if (len == SOCKET_ERROR) {
						ErrorProc(soc[val].socket, "send()");
					}

					soc[val].flg = true;
				}
			}
			for (int i = 0; i < MAXSOC; i++) {
				if (FD_ISSET(soc[i].socket, &read_fds)) {
					printf("soc[%d].socket: %d\n",i, soc[i].socket);
					len = recv(soc[i].socket, msg, MSG_SIZE, 0);
					if (len == SOCKET_ERROR) {
						ErrorProc(soc[i].socket, "recv()");
					}
					msg[len] = '\0';
					printf("��M: %s\n", msg);
					len = send(soc[i].socket, msg, (int)strlen(msg), 0);
					if (len == SOCKET_ERROR) {
						ErrorProc(soc[i].socket, "send()");
					}
					printf("���M: %s\n", msg);
					if (strncmp(msg, "bye", 3) == 0) { soc[i].flg = false; }
				}

			}

			//}else {

			//	printf("elseFD_ISSET");

			//}

			timecnt = 0;
	
		}
		else {
			printf("��M�Ȃ��B:�@���Ɓ@%d�@�b�ŏI������B\n", (time*timeout.tv_sec) - (timecnt*timeout.tv_sec));
			if (time <= timecnt) {
				goto END;
			}
			else {
				timecnt++;
			}
		}
	}
	
END:

	closesocket(s1);

	for (int i = 0; i < MAXSOC; i++) {
		closesocket(soc[i].socket);
	}

	closesocket(sa);
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