/*** ���⼭���� �� å�� ��� �������� �������� �����Ͽ� ����ϴ� �ڵ��̴�. ***/

#define _CRT_SECURE_NO_WARNINGS // ���� C �Լ� ��� �� ��� ����
#define _WINSOCK_DEPRECATED_NO_WARNINGS // ���� ���� API ��� �� ��� ����

#include <winsock2.h> // ����2 ���� ���
#include <ws2tcpip.h> // ����2 Ȯ�� ���

#include <tchar.h> // _T(), ...
#include <stdio.h> // printf(), ...
#include <stdlib.h> // exit(), ...
#include <string.h> // strncpy(), ...

#pragma comment(lib, "ws2_32") // ws2_32.lib ��ũ

// ���� �Լ� ���� ��� �� ����
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

// ���� �Լ� ���� ���
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

// ���� �Լ� ���� ���
void err_display(int errcode)
{
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, errcode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf, 0, NULL);
	printf("[����] %s\n", (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

/*** ��������� �� å�� ��� �������� �������� �����Ͽ� ����ϴ� �ڵ��̴�. ***/
/*** 2�� ������ �������� Common.h�� �����ϴ� ������� �� �ڵ带 ����Ѵ�.  ***/

#include "resource.h"
#include <commctrl.h>

char SERVERIP[16] = "127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    50

// ��ȭ���� ���ν���
INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
// ����Ʈ ��Ʈ�� ��� �Լ�
void DisplayText(const char* fmt, ...);
// ���� �Լ� ���� ���
void DisplayError(const char* msg);
// ���� ��� ������ �Լ�
DWORD WINAPI ClientMain(LPVOID arg);

SOCKET sock; // ����
char buf[BUFSIZE + 1];
HWND hSendButton; // ������ ��ư
HWND hEdit1, hEdit2, hProgress;
char filename[MAX_PATH] = { 0 };

// ��ȭ���� ���ν���
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
		SendMessage(hProgress, PBM_SETPOS, 0, 0); // ���� ��ġ 0%
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
			EndDialog(hDlg, IDCANCEL); // ��ȭ���� �ݱ�
			closesocket(sock); // ���� �ݱ�
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}
// ����Ʈ ��Ʈ�� ��� �Լ�
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
// ���� �Լ� ���� ���
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

	// ���� ����
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// ���� ���� ����
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("connect()");

	// ���� ����
	FILE* file = fopen(filename, "rb");
	if (file == NULL) {
		MessageBoxA(NULL, "������ �� �� �����ϴ�.", "����", MB_ICONERROR);
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	// ���� ũ�� ����
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

	// ���� ������ ����
	char buf[BUFSIZE];
	size_t len;
	long int total_sent = 0;
	DisplayText("���� ���Դϴ�... \r\n");
	while ((len = fread(buf, 1, BUFSIZE, file)) > 0) {
		retval = send(sock, buf, (int)len, 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
			break;
		}
		total_sent += retval;
		int progress = (int)((total_sent * 100) / filesize);
		SendMessage(hProgress, PBM_SETPOS, progress, 0); // ���α׷��� �� ��ġ ����
	}

	// ���� �ݱ�
	fclose(file);

	DisplayText("\r\n���� ���� �Ϸ�. ��ɾ �Է��ϼ���: \r\n");

	closesocket(sock);
	WSACleanup();
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	// ����� �μ� �Ľ�
	char* token = strtok(lpCmdLine, " ");
	if (token != NULL) strcpy(filename, token);
	token = strtok(NULL, " ");
	if (token != NULL) strncpy(SERVERIP, token, sizeof(SERVERIP) - 1);

	// ���� �̸� ��ȿ�� �˻�
	if (strlen(filename) == 0) {
		MessageBoxA(NULL, "���� �̸��� �����ؾ� �մϴ�.", "����", MB_ICONERROR);
		return 1;
	}

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	HANDLE hClientThread = CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);
	if (hClientThread == NULL) {
		MessageBoxA(NULL, "ClientMain ������ ���� ����", "����", MB_ICONERROR);
		WSACleanup();
		return 1;
	}

	// ��ȭ���� ����
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	// ���� ��� ������ ���� ���
	WaitForSingleObject(hClientThread, INFINITE);

	// ������ �ڵ� �ݱ� �� ���� ����
	CloseHandle(hClientThread);
	WSACleanup();
	return 0;
}
