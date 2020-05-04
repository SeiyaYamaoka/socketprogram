#pragma comment(lib,"user32.lib")
#pragma comment(lib,"wsock32.lib")

#define STRICT

#include <winsock.h>
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <crtdbg.h>
#include "chat.h"

//関数プロトタイプ宣言
BOOL CALLBACK InitDlgFunc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DialogFunc(HWND, UINT, WPARAM, LPARAM);
void ErrorProc(SOCKET sock,const char *ApiName, HWND hWnd);
void Display(HWND hWnd, const char *str);
bool AddEdBoxString(HWND hWnd, const char *string);

#define SD_RECEIVE		0	//shutdown()の引数マクロ定義（winsock2では不要)
#define SD_SEND			1	//受信しない
#define SD_BOTH			2	//送受信しない

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

socinf soc[MAXSOC]; //複数受信用

char name[255] = "";
char anauns[255] = "";

char chsend[BUF_SIZE];


//メイン関数
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
		MessageBox(NULL, (LPCSTR)"ライブラリの初期化に失敗しました", (LPCSTR)"sample", MB_OK);
		return 1;
	}
	if (wsadata.wVersion != wVerReq) {
		MessageBox(NULL, (LPCSTR)"要求したバージョンがサポートされていません", (LPCSTR)"sample", MB_OK);
		WSACleanup();
		return 1;
	}


	//dialogを開く
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

//起動初期かダイアログボックスのメッセージ処理関数
BOOL CALLBACK InitDlgFunc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND hwrb;
	switch (message) {
	case WM_INITDIALOG:
		//サーバーのラジオボタンをオフにする
		SendDlgItemMessage(hWnd, IDD_RB_SV, BM_SETCHECK, 0, 0);
		//クライアントのラジオボタンをオンにする
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
			

			//ホスト名とポート番号をチェックして取得
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
//チャット用ダイアログボックスのメッセージ処理関数
BOOL CALLBACK DialogFunc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_INITDIALOG:
		switch (InitFlag) {
		case SERVER:
			//サーバー初期化
			//socket()

			//初期化
			for (int i = 0; i < MAXSOC; i++) {
				soc[i].use = false;
			}

			sa = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (sa == INVALID_SOCKET) {
				ErrorProc(sa,"SERVERsocket()",hWnd);
			}

			//ソケットで受け付け受付に将来誰か来たら教えてそれか閉じたら教えてSM＿ASYNCに飛ぶ
			if (WSAAsyncSelect(sa, hWnd, SM_ASYNC, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR) {
				ErrorProc(sa, "wdaasyncselect ()", hWnd);
			}

			//bind
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
			addr.sin_port = htons(atoi(portnum));
			if (bind(sa, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
				//エラー処理
				ErrorProc(sa, "bind()",hWnd);
			}
			//listen
			if (listen(sa, SOMAXCONN) == SOCKET_ERROR) {
				//エラー処理
				ErrorProc(sa, "listen()",hWnd);
			}

			break;
		case CLIENT:
			//クライアント初期化
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
				//エラー処理
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
			//送信処理
			int count;
			int len, getlen;
			int lineindex;
			
			TCHAR *temp;
			//行数取得
			count = SendDlgItemMessage(hWnd, IDD_EB_SEND, EM_GETLINECOUNT, (WPARAM)0, (LPARAM)0);
			
			//その行の先頭文字のインデックス
			lineindex = SendDlgItemMessage(hWnd, IDD_EB_SEND, EM_LINEINDEX, (WPARAM)0, (LPARAM)0);
			//行の文字数
			len = SendDlgItemMessage(hWnd, IDD_EB_SEND, EM_LINELENGTH, (WPARAM)lineindex, (LPARAM)0);

			if(len != 0){
				temp = new TCHAR[len + 1];
				//重要：バッファの長さを２バイト使用して指定すること
				*(WORD *)temp = (WORD)len;
				//１行取得(Nullターミネータなし）
				getlen = SendDlgItemMessage(hWnd, IDD_EB_SEND, EM_GETLINE, (WPARAM)0, (LPARAM)temp);

				//Nullターミネータ追加
				temp[len] = TEXT('\0');

				/* ------------------
				//チャット前の名前
				------------------ */
				strcat_s(chsend, name);
				strcat_s(chsend, ">> ");
				strcat_s(chsend, temp);

				//strcat_s(name,temp);
				for (int i = 0; i < MAXSOC; i++) {
					if (soc[i].use == true) {
						len = send(soc[i].socket, chsend, (int)strlen(chsend), 0);
						if (len == SOCKET_ERROR) {
							ErrorProc(soc[i].socket, "メッセージを送れません", hWnd);
						}
					}
				}
				if (InitFlag == CLIENT) {
					len = send(s, chsend, (int)strlen(chsend), 0);
					if (len == SOCKET_ERROR) {
						ErrorProc(s, "メッセージを送れません", hWnd);
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
			//ソケットを閉じる処理
			EndDialog(hWnd, 1);
			closesocket(s);
			closesocket(sa);
			return TRUE;
			break;
		}
		break;
	case SM_ASYNC://ネットワークイベント発生
		//ネットワークイベント処理

		int nErrorCode = WSAGETSELECTERROR(lParam); // エラーコードを取り出す
		int nEventCode = WSAGETSELECTEVENT(lParam);//イベントコードを取り出す
		if (nErrorCode != 0) { ErrorProc(s, "サーバと接続できません", hWnd); }
		switch (nEventCode) {
		case FD_ACCEPT:


			for (int i = 0; i < MAXSOC; i++) {
				if (soc[i].use == false) {
					val = i;
					break;
				}
				max = i;
			}
			if (max == MAXSOC - 1) { //用意しているソケットの最大値を超えた
				ErrorProc(sa, "最大利用数を超えました。", hWnd);
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


			AddEdBoxString(hWnd, "クライアントと接続しました");

			//len = send(s, hello, (int)strlen(hello), 0);

			//if (len == SOCKET_ERROR) {
			//	ErrorProc(s, "send()", hWnd);
			//}
			//s = accept(sa, ~~
			//sa　 sが生まれたから以下のことがあったら教えて
			//ソケットｓにFD_～～～が発生したらSM_ASCYNCをhWndのコールバック関数に送れ
			//１００３５　WASEWOULDBLOCK　これは無視　まだ届いてないよってやつFD_RIGHT　＝　ON　次の送信が
			//キャンセルが起こったら抜ける　
			//ソケット番号はwParamに入っている、sa の代わりにwParamを使っても　受信の時にwParamを使ってもいい　どのソケットに届いているのか
			//複数のソケットがつながっているときは楽になるかなと・
			if (WSAAsyncSelect(soc[val].socket, hWnd, SM_ASYNC, FD_READ | FD_WRITE | FD_CLOSE) == SOCKET_ERROR) {
				ErrorProc(soc[val].socket, "WSAAsyncSelect FD_ACCEPT", hWnd);
			}
			break;
		case FD_CONNECT: //サーバーに接続されたことをユーザに通知
			AddEdBoxString(hWnd, "サーバと接続されました");
			//char tmp[128];
			//strcat_s(tmp, name);
			//strcat_s(tmp, "と接続しました");
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
		case FD_READ://recv()と表示
			
			len = recv(wParam, SBuf, SBUF_SIZE, 0);
			if (len == SOCKET_ERROR) {
				ErrorProc(wParam, "recv()",hWnd);
			}
			
			//もし自分がサーバーなら受けっとったソケット以外のクライアントにも送られてきたメッセージを送る
			for (int i = 0; i < MAXSOC; i++) {
				if (soc[i].use == true && soc[i].socket != wParam) {
					clensend = send(soc[i].socket, SBuf, len, 0);
					if (clensend == SOCKET_ERROR) {
						ErrorProc(soc[i].socket, "メッセージを送れません", hWnd);
					}
				}
			}
			

			SBuf[len] = '\0';
			//printf("受信: %s\n", msg);
			AddEdBoxString(hWnd, (LPCSTR)SBuf);
			//len = send(s, SBuf, (int)strlen(SBuf), 0);
			//if (len == SOCKET_ERROR) {
			//	ErrorProc(s, "send()",hWnd);
			//}
			//printf("送信: %s\n", msg);


			break;
		case FD_WRITE://送信が可能になったことをユーザに通知、送信許可フラグをONにする
			break;
		case FD_CLOSE://コネクションが切れたことをユーザに通知
			AddEdBoxString(hWnd, "接続が切れました。");
			break;

		}

		return TRUE;
		break;

	}
	return FALSE;
}

void ErrorProc(SOCKET sock,const char *ApiName, HWND hWnd) {
	//エラー処理
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
	//エディットボックスに文字列を追加する関数
	int edlen;
	//GetDlgItemText(hWnd, IDD_EB_HOST, (LPSTR)hostname, sizeof hostname);
	edlen = SendDlgItemMessage(hWnd, IDD_EB_CHAT, WM_GETTEXTLENGTH, 0, 0);
	//一番最後にカーソルを移動
	SendDlgItemMessage(hWnd, IDD_EB_CHAT, EM_SETSEL, (WPARAM)edlen, (LPARAM)edlen);
	//文字列を挿入（置き換え）
	SendDlgItemMessage(hWnd, IDD_EB_CHAT, EM_REPLACESEL, (WPARAM)false, (LPARAM)string);
	//改行を追加
	SendDlgItemMessage(hWnd, IDD_EB_CHAT, EM_REPLACESEL, (WPARAM)false, (LPARAM)TEXT("\r\n"));

	return true;

}