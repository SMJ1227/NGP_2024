/*** 여기서부터 이 책의 모든 예제에서 공통으로 포함하여 사용하는 코드이다. ***/

#define _CRT_SECURE_NO_WARNINGS // 구형 C 함수 사용 시 경고 끄기
#define _WINSOCK_DEPRECATED_NO_WARNINGS // 구형 소켓 API 사용 시 경고 끄기

#include <winsock2.h> // 윈속2 메인 헤더
#include <ws2tcpip.h> // 윈속2 확장 헤더

#include <tchar.h> // _T(), ...
#include <stdio.h> // printf(), ...
#include <stdlib.h> // exit(), ...
#include <string.h> // strncpy(), ...

#pragma comment(lib, "ws2_32") // ws2_32.lib 링크

// 소켓 함수 오류 출력 후 종료
void err_quit(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf, 0, NULL);
	MessageBoxA(NULL, (const char*)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// 소켓 함수 오류 출력
void err_display(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf, 0, NULL);
	printf("[%s] %s\n", msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// 소켓 함수 오류 출력
void err_display(int errcode)
{
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, errcode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf, 0, NULL);
	printf("[오류] %s\n", (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

/*** 여기까지가 이 책의 모든 예제에서 공통으로 포함하여 사용하는 코드이다. ***/
/*** 2장 이후의 예제들은 Common.h를 포함하는 방식으로 이 코드를 사용한다.  ***/

#include "resource.h"
#include <commctrl.h>

char SERVERIP[16] = "127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    50

// 대화상자 프로시저
INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
// 에디트 컨트롤 출력 함수
void DisplayText(const char* fmt, ...);
// 소켓 함수 오류 출력
void DisplayError(const char* msg);
// 소켓 통신 스레드 함수
DWORD WINAPI ClientMain(LPVOID arg);

SOCKET sock; // 소켓
char buf[BUFSIZE + 1];
HWND hSendButton; // 보내기 버튼
HWND hEdit1, hEdit2, hProgress;
char filename[MAX_PATH] = { 0 };

// 대화상자 프로시저
INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		hEdit1 = GetDlgItem(hDlg, IDC_EDIT1);
		hEdit2 = GetDlgItem(hDlg, IDC_EDIT2);
		hSendButton = GetDlgItem(hDlg, IDOK);
		hProgress = GetDlgItem(hDlg, IDC_PROGRESS1);
		SendMessage(hEdit1, EM_SETLIMITTEXT, BUFSIZE, 0);
		SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100)); // 0 ~ 100%
		SendMessage(hProgress, PBM_SETPOS, 0, 0); // 시작 위치 0%
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			GetDlgItemTextA(hDlg, IDC_EDIT1, buf, BUFSIZE + 1);
			DisplayText("%s\r\n", buf);
			if (strncmp(buf, "exit", 4) == 0) {
				EnableWindow(hSendButton, TRUE);
				EndDialog(hDlg, IDCANCEL);
				closesocket(sock);
			}
			else {
				SetFocus(hEdit1);
				SendMessage(hEdit1, EM_SETSEL, 0, -1);
			}
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL); // 대화상자 닫기
			closesocket(sock); // 소켓 닫기
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}
// 에디트 컨트롤 출력 함수
void DisplayText(const char* fmt, ...) {
	va_list arg;
	va_start(arg, fmt);
	char cbuf[BUFSIZE * 2];
	vsprintf(cbuf, fmt, arg);
	va_end(arg);

	int nLength = GetWindowTextLength(hEdit2);
	SendMessage(hEdit2, EM_SETSEL, nLength, nLength);
	SendMessageA(hEdit2, EM_REPLACESEL, FALSE, (LPARAM)cbuf);

}
// 소켓 함수 오류 출력
void DisplayError(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf, 0, NULL);
	DisplayText("[%s] %s\r\n", msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

DWORD WINAPI ClientMain(LPVOID arg) {
	int retval;

	// 소켓 생성
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// 서버 연결 설정
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("connect()");

	// 파일 오픈
	FILE* file = fopen(filename, "rb");
	if (file == NULL) {
		MessageBoxA(NULL, "파일을 열 수 없습니다.", "오류", MB_ICONERROR);
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	// 파일 크기 전송
	fseek(file, 0, SEEK_END);
	long int filesize = ftell(file);
	fseek(file, 0, SEEK_SET);
	retval = send(sock, (char*)&filesize, sizeof(long int), 0);
	if (retval == SOCKET_ERROR) {
		err_display("send()");
		fclose(file);
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	// 파일 데이터 전송
	char buf[BUFSIZE];
	size_t len;
	long int total_sent = 0;
	DisplayText("전송 중입니다... \r\n");
	while ((len = fread(buf, 1, BUFSIZE, file)) > 0) {
		retval = send(sock, buf, (int)len, 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
			break;
		}
		total_sent += retval;
		int progress = (int)((total_sent * 100) / filesize);
		SendMessage(hProgress, PBM_SETPOS, progress, 0); // 프로그래스 바 위치 설정
	}

	// 파일 닫기
	fclose(file);

	DisplayText("\r\n파일 전송 완료. 명령어를 입력하세요: \r\n");

	closesocket(sock);
	WSACleanup();
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	// 명령줄 인수 파싱
	char* token = strtok(lpCmdLine, " ");
	if (token != NULL) strcpy(filename, token);
	token = strtok(NULL, " ");
	if (token != NULL) strncpy(SERVERIP, token, sizeof(SERVERIP) - 1);

	// 파일 이름 유효성 검사
	if (strlen(filename) == 0) {
		MessageBoxA(NULL, "파일 이름을 제공해야 합니다.", "오류", MB_ICONERROR);
		return 1;
	}

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	HANDLE hClientThread = CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);
	if (hClientThread == NULL) {
		MessageBoxA(NULL, "ClientMain 스레드 생성 실패", "오류", MB_ICONERROR);
		WSACleanup();
		return 1;
	}

	// 대화상자 실행
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	// 소켓 통신 스레드 종료 대기
	WaitForSingleObject(hClientThread, INFINITE);

	// 스레드 핸들 닫기 및 윈속 종료
	CloseHandle(hClientThread);
	WSACleanup();
	return 0;
}
