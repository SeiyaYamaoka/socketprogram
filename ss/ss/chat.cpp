#pragma comment(lib,"user32.lib")
#pragma comment(lib,"wsock32.lib")

#define STRICT

#include <winsock.h>
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <crtdbg.h>
#include "chat.h"

//�֐��v���g�^�C�v�錾
BOOL CALLBACK InitDlgFunc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DialogFunc(HWND, UINT, WPARAM, LPARAM);
void ErrorProc(SOCKET sock,const char *ApiName, HWND hWnd);
void Display(HWND hWnd, const char *str);
bool AddEdBoxString(HWND hWnd, const char *string);

#define SD_RECEIVE		0	//shutdown()�̈����}�N����`�iwinsock2�ł͕s�v)
#define SD_SEND			1	//��M���Ȃ�
#define SD_BOTH			2	//����M���Ȃ�

#define SERVER			0
#define CLIENT			1
int InitFlag = CLIENT;

#define SEND_DISABLE 0
#define SEND_ENABLE 1
int SendFlag = SEND_DISABLE;

#define STATE_CLOSE 0
#define STATE_OPEN 1
int saState = STATE_CLOSE;
int sState = STATE_CLOSE;

#define BUF_SIZE 1024
#define SBUF_SIZE 4096
#define RBUF_SIZE 4096
char SBuf[SBUF_SIZE + 1];
char Rbuf[RBUF_SIZE + 1];
char hostname[BUF_SIZE];
char portnum[BUF_SIZE];
unsigned long ipv4_addr;
unsigned short port;
SOCKET sa;
SOCKET s;
struct sockaddr_in addr;

SOCKET reseption;

int len;

int clensend;

int val;
int max = 0;

#define MAXSOC 100

struct socinf {
	SOCKET socket;
	bool use;
};

socinf soc[MAXSOC]; //������M�p

char name[255] = "";
char anauns[255] = "";

char chsend[BUF_SIZE];


//���C���֐�
int WINAPI WinMain(
	HINSTANCE hThisInst, 
	HINSTANCE hPrevInst,
	LPSTR lpszArgs,
	int hWinMode
) {
	WORD wVerReq;
	WSADATA wsadata;
#define MAJOR_VERSION_REQUIRED 1
#define MINOR_VERSION_REQUIRED 1
	wVerReq = MAKEWORD(MAJOR_VERSION_REQUIRED, MINOR_VERSION_REQUIRED);
	if (WSAStartup(wVerReq, &wsadata) != 0) {
		MessageBox(NULL, (LPCSTR)"���C�u�����̏������Ɏ��s���܂���", (LPCSTR)"sample", MB_OK);
		return 1;
	}
	if (wsadata.wVersion != wVerReq) {
		MessageBox(NULL, (LPCSTR)"�v�������o�[�W�������T�|�[�g����Ă��܂���", (LPCSTR)"sample", MB_OK);
		WSACleanup();
		return 1;
	}


	//dialog���J��
	while(1) {
		if (DialogBox(hThisInst, (LPCSTR)"IDD_INIT_DIALOG", NULL, InitDlgFunc) == 0) {
			DialogBox(hThisInst, (LPCSTR)"IDD_DIALOG_MAIN", NULL, DialogFunc);
		}
		else {
			break;
		}
	}
	WSACleanup();
	return 0;
	
}

//�N���������_�C�A���O�{�b�N�X�̃��b�Z�[�W�����֐�
BOOL CALLBACK InitDlgFunc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND hwrb;
	switch (message) {
	case WM_INITDIALOG:
		//�T�[�o�[�̃��W�I�{�^�����I�t�ɂ���
		SendDlgItemMessage(hWnd, IDD_RB_SV, BM_SETCHECK, 0, 0);
		//�N���C�A���g�̃��W�I�{�^�����I���ɂ���
		SendDlgItemMessage(hWnd, IDD_RB_CL, BM_SETCHECK, 1, 0);
		SetDlgItemText(hWnd, IDD_EB_HOST, (LPCSTR)"127.0.0.1");
		SetDlgItemText(hWnd, IDD_EB_PORT, (LPCSTR)"49152");
		SetDlgItemText(hWnd, IDD_EB_NAME, (LPSTR)name);
		return TRUE;
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDD_RB_SV:
			hwrb = GetDlgItem(hWnd, IDD_EB_HOST);
			EnableWindow(hwrb, 0);
			return TRUE;
			break;
		case IDD_RB_CL:
			hwrb = GetDlgItem(hWnd, IDD_EB_HOST);
			EnableWindow(hwrb, 1);
			return TRUE;
			break;
		case IDD_PB_OK:
			GetDlgItemText(hWnd, IDD_EB_HOST, (LPSTR)hostname, sizeof hostname);
			GetDlgItemText(hWnd, IDD_EB_PORT, (LPSTR)portnum, sizeof portnum);

			GetDlgItemText(hWnd, IDD_EB_NAME, (LPSTR)name, sizeof name);
			

			//�z�X�g���ƃ|�[�g�ԍ����`�F�b�N���Ď擾
			InitFlag = SendDlgItemMessage(hWnd, IDD_RB_SV, BM_GETCHECK, 0, 0) ? SERVER : CLIENT;
			
			EndDialog(hWnd, 0);

			return TRUE;
			break;

		case IDCANCEL:
			EndDialog(hWnd, 1);
			return TRUE;
			break;
		}
		break;
	}
	return FALSE;
}
//�`���b�g�p�_�C�A���O�{�b�N�X�̃��b�Z�[�W�����֐�
BOOL CALLBACK DialogFunc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_INITDIALOG:
		switch (InitFlag) {
		case SERVER:
			//�T�[�o�[������
			//socket()

			//������
			for (int i = 0; i < MAXSOC; i++) {
				soc[i].use = false;
			}

			sa = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (sa == INVALID_SOCKET) {
				ErrorProc(sa,"SERVERsocket()",hWnd);
			}

			//�\�P�b�g�Ŏ󂯕t����t�ɏ����N�������狳���Ă��ꂩ�����狳����SM�QASYNC�ɔ��
			if (WSAAsyncSelect(sa, hWnd, SM_ASYNC, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR) {
				ErrorProc(sa, "wdaasyncselect ()", hWnd);
			}

			//bind
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
			addr.sin_port = htons(atoi(portnum));
			if (bind(sa, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
				//�G���[����
				ErrorProc(sa, "bind()",hWnd);
			}
			//listen
			if (listen(sa, SOMAXCONN) == SOCKET_ERROR) {
				//�G���[����
				ErrorProc(sa, "listen()",hWnd);
			}

			break;
		case CLIENT:
			//�N���C�A���g������
			//socket()
			s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (s == INVALID_SOCKET) {
				ErrorProc(s, "CLIENTsocket()", hWnd);
			}
			if (WSAAsyncSelect(s, hWnd, SM_ASYNC, FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE) == SOCKET_ERROR) {

			}
			//connect
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = inet_addr(hostname);
			addr.sin_port = htons(atoi(portnum));
			if (connect(s, (struct sockaddr *)&addr, sizeof addr) == SOCKET_ERROR) {
				//�G���[����
				_RPTN(_CRT_WARN, "hostname %s\n", hostname);
				_RPTN(_CRT_WARN, "portnum %d\n", atoi(portnum));
				_RPTN(_CRT_WARN, "Error Code = %d\n", WSAGetLastError());
				ErrorProc(s, "CLIENTconnect()", hWnd);

			}

			break;
		}
		return TRUE;
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDD_PB_SEND:
			//���M����
			int count;
			int len, getlen;
			int lineindex;
			
			TCHAR *temp;
			//�s���擾
			count = SendDlgItemMessage(hWnd, IDD_EB_SEND, EM_GETLINECOUNT, (WPARAM)0, (LPARAM)0);
			
			//���̍s�̐擪�����̃C���f�b�N�X
			lineindex = SendDlgItemMessage(hWnd, IDD_EB_SEND, EM_LINEINDEX, (WPARAM)0, (LPARAM)0);
			//�s�̕�����
			len = SendDlgItemMessage(hWnd, IDD_EB_SEND, EM_LINELENGTH, (WPARAM)lineindex, (LPARAM)0);

			if(len != 0){
				temp = new TCHAR[len + 1];
				//�d�v�F�o�b�t�@�̒������Q�o�C�g�g�p���Ďw�肷�邱��
				*(WORD *)temp = (WORD)len;
				//�P�s�擾(Null�^�[�~�l�[�^�Ȃ��j
				getlen = SendDlgItemMessage(hWnd, IDD_EB_SEND, EM_GETLINE, (WPARAM)0, (LPARAM)temp);

				//Null�^�[�~�l�[�^�ǉ�
				temp[len] = TEXT('\0');

				/* ------------------
				//�`���b�g�O�̖��O
				------------------ */
				strcat_s(chsend, name);
				strcat_s(chsend, ">> ");
				strcat_s(chsend, temp);

				//strcat_s(name,temp);
				for (int i = 0; i < MAXSOC; i++) {
					if (soc[i].use == true) {
						len = send(soc[i].socket, chsend, (int)strlen(chsend), 0);
						if (len == SOCKET_ERROR) {
							ErrorProc(soc[i].socket, "���b�Z�[�W�𑗂�܂���", hWnd);
						}
					}
				}
				if (InitFlag == CLIENT) {
					len = send(s, chsend, (int)strlen(chsend), 0);
					if (len == SOCKET_ERROR) {
						ErrorProc(s, "���b�Z�[�W�𑗂�܂���", hWnd);
					}
				}
				_RPTN(_CRT_WARN, "initflg %d \n", InitFlag);
				AddEdBoxString(hWnd, (LPCSTR)chsend);

				memset(&chsend, '\0', sizeof(chsend));
				delete[] temp;

				SetWindowText(GetDlgItem(hWnd, IDD_EB_SEND), "");
			}
			//_RPTN(_CRT_WARN, "len %d \n", len);
			


			return TRUE;
			break;
		case IDCANCEL:
			//�\�P�b�g����鏈��
			EndDialog(hWnd, 1);
			closesocket(s);
			closesocket(sa);
			return TRUE;
			break;
		}
		break;
	case SM_ASYNC://�l�b�g���[�N�C�x���g����
		//�l�b�g���[�N�C�x���g����

		int nErrorCode = WSAGETSELECTERROR(lParam); // �G���[�R�[�h�����o��
		int nEventCode = WSAGETSELECTEVENT(lParam);//�C�x���g�R�[�h�����o��
		if (nErrorCode != 0) { ErrorProc(s, "�T�[�o�Ɛڑ��ł��܂���", hWnd); }
		switch (nEventCode) {
		case FD_ACCEPT:


			for (int i = 0; i < MAXSOC; i++) {
				if (soc[i].use == false) {
					val = i;
					break;
				}
				max = i;
			}
			if (max == MAXSOC - 1) { //�p�ӂ��Ă���\�P�b�g�̍ő�l�𒴂���
				ErrorProc(sa, "�ő嗘�p���𒴂��܂����B", hWnd);
			}
			else {
				len = sizeof addr;
				soc[val].socket = accept(sa, (struct sockaddr*)&addr, &len);
				if (soc[val].socket == INVALID_SOCKET) {
					ErrorProc(sa, "accept()", hWnd);
				}
				soc[val].use = true;
			}


			

			//closesocket(sa);

			//printf("Client IP	:%s\n", inet_ntoa(addrc.sin_addr));
			//printf("Client Port	:%hu\n", ntohs(addrc.sin_port));


			AddEdBoxString(hWnd, "�N���C�A���g�Ɛڑ����܂���");

			//len = send(s, hello, (int)strlen(hello), 0);

			//if (len == SOCKET_ERROR) {
			//	ErrorProc(s, "send()", hWnd);
			//}
			//s = accept(sa, ~~
			//sa�@ s�����܂ꂽ����ȉ��̂��Ƃ��������狳����
			//�\�P�b�g����FD_�`�`�`������������SM_ASCYNC��hWnd�̃R�[���o�b�N�֐��ɑ���
			//�P�O�O�R�T�@WASEWOULDBLOCK�@����͖����@�܂��͂��ĂȂ�����Ă��FD_RIGHT�@���@ON�@���̑��M��
			//�L�����Z�����N�������甲����@
			//�\�P�b�g�ԍ���wParam�ɓ����Ă���Asa �̑����wParam���g���Ă��@��M�̎���wParam���g���Ă������@�ǂ̃\�P�b�g�ɓ͂��Ă���̂�
			//�����̃\�P�b�g���Ȃ����Ă���Ƃ��͊y�ɂȂ邩�ȂƁE
			if (WSAAsyncSelect(soc[val].socket, hWnd, SM_ASYNC, FD_READ | FD_WRITE | FD_CLOSE) == SOCKET_ERROR) {
				ErrorProc(soc[val].socket, "WSAAsyncSelect FD_ACCEPT", hWnd);
			}
			break;
		case FD_CONNECT: //�T�[�o�[�ɐڑ����ꂽ���Ƃ����[�U�ɒʒm
			AddEdBoxString(hWnd, "�T�[�o�Ɛڑ�����܂���");
			//char tmp[128];
			//strcat_s(tmp, name);
			//strcat_s(tmp, "�Ɛڑ����܂���");
			//
			//len = send(s, tmp, (int)strlen(tmp), 0);
			//if (len == SOCKET_ERROR) {
			//	ErrorProc(s, "send()", hWnd);
			//}
		
			//len = recv(s, SBuf, SBUF_SIZE, 0);
			//if (len == SOCKET_ERROR) {
			//	ErrorProc(s, "recv()",hWnd);
			//}
			//SBuf[len] = '\0';
			//AddEdBoxString(hWnd, (LPCSTR)SBuf);
			/*if (strncmp(msg, "bye", 3) == 0) { break; }*/

			break;
		case FD_READ://recv()�ƕ\��
			
			len = recv(wParam, SBuf, SBUF_SIZE, 0);
			if (len == SOCKET_ERROR) {
				ErrorProc(wParam, "recv()",hWnd);
			}
			
			//�����������T�[�o�[�Ȃ�󂯂��Ƃ����\�P�b�g�ȊO�̃N���C�A���g�ɂ������Ă������b�Z�[�W�𑗂�
			for (int i = 0; i < MAXSOC; i++) {
				if (soc[i].use == true && soc[i].socket != wParam) {
					clensend = send(soc[i].socket, SBuf, len, 0);
					if (clensend == SOCKET_ERROR) {
						ErrorProc(soc[i].socket, "���b�Z�[�W�𑗂�܂���", hWnd);
					}
				}
			}
			

			SBuf[len] = '\0';
			//printf("��M: %s\n", msg);
			AddEdBoxString(hWnd, (LPCSTR)SBuf);
			//len = send(s, SBuf, (int)strlen(SBuf), 0);
			//if (len == SOCKET_ERROR) {
			//	ErrorProc(s, "send()",hWnd);
			//}
			//printf("���M: %s\n", msg);


			break;
		case FD_WRITE://���M���\�ɂȂ������Ƃ����[�U�ɒʒm�A���M���t���O��ON�ɂ���
			break;
		case FD_CLOSE://�R�l�N�V�������؂ꂽ���Ƃ����[�U�ɒʒm
			AddEdBoxString(hWnd, "�ڑ����؂�܂����B");
			break;

		}

		return TRUE;
		break;

	}
	return FALSE;
}

void ErrorProc(SOCKET sock,const char *ApiName, HWND hWnd) {
	//�G���[����
	/*Display(hWnd, ApiName);*/
	if(WSAGetLastError() != 10035){
		MessageBox(hWnd, (LPCSTR)ApiName, ("Error"), MB_ICONSTOP);
		EndDialog(hWnd, 1);
		closesocket(s);
		closesocket(sa);


		//Display(hWnd, ApiName);
	}
}
void Display(HWND hWnd,const char *str) {
	SendDlgItemMessage(hWnd, IDD_EB_CHAT, EM_REPLACESEL, 0, (LPARAM)((LPSTR)str));
	//MessageBox(hWnd, _T(str), _T("Err"), MB_ICONSTOP);
}
bool AddEdBoxString(HWND hWnd, const char *string) {
	//�G�f�B�b�g�{�b�N�X�ɕ������ǉ�����֐�
	int edlen;
	//GetDlgItemText(hWnd, IDD_EB_HOST, (LPSTR)hostname, sizeof hostname);
	edlen = SendDlgItemMessage(hWnd, IDD_EB_CHAT, WM_GETTEXTLENGTH, 0, 0);
	//��ԍŌ�ɃJ�[�\�����ړ�
	SendDlgItemMessage(hWnd, IDD_EB_CHAT, EM_SETSEL, (WPARAM)edlen, (LPARAM)edlen);
	//�������}���i�u�������j
	SendDlgItemMessage(hWnd, IDD_EB_CHAT, EM_REPLACESEL, (WPARAM)false, (LPARAM)string);
	//���s��ǉ�
	SendDlgItemMessage(hWnd, IDD_EB_CHAT, EM_REPLACESEL, (WPARAM)false, (LPARAM)TEXT("\r\n"));

	return true;

}