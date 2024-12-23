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

#define SERVERPORT 9000
#define BUFSIZE    512
#define MAX_FILES 100

#include <windows.h> // windows 관련 함수 포함

// 윈도우 프로시저
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
// 에디트 컨트롤 출력 함수
void DisplayText(int line, const char* fmt, ...);
// 소켓 함수 오류 출력
void DisplayError(const char* msg);
// 소켓 통신 스레드 함수
DWORD WINAPI ServerMain(LPVOID arg);
DWORD WINAPI ProcessClient(LPVOID arg);

HINSTANCE hInst; // 인스턴스 핸들
HWND hEdit; // 에디트 컨트롤
CRITICAL_SECTION cs; // 임계 영역

int file_count = 0;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	hInst = hInstance;
	InitializeCriticalSection(&cs);

	// 윈도우 클래스 등록
	WNDCLASS wndclass;
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = _T("MyWndClass");
	if (!RegisterClass(&wndclass)) return 1;

	// 윈도우 생성
	HWND hWnd = CreateWindow(_T("MyWndClass"), _T("TCP 서버"),
		WS_OVERLAPPEDWINDOW, 0, 0, 800, 600,
		NULL, NULL, hInstance, NULL);
	if (hWnd == NULL) return 1;
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// 소켓 통신 스레드 생성
	CreateThread(NULL, 0, ServerMain, NULL, 0, NULL);

	// 메시지 루프
	MSG msg;
	while (GetMessage(&msg, 0, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	DeleteCriticalSection(&cs);
	return (int)msg.wParam;
}

// 윈도우 프로시저
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		hEdit = CreateWindow(_T("edit"), NULL,
			WS_CHILD | WS_VISIBLE | WS_HSCROLL |
			WS_VSCROLL | ES_AUTOHSCROLL |
			ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY,
			0, 0, 0, 0, hWnd, (HMENU)100, hInst, NULL);
		return 0;
	case WM_SIZE:
		MoveWindow(hEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		return 0;
	case WM_SETFOCUS:
		SetFocus(hEdit);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

double last_percentage = -1;

void DisplayText(int line, const char* fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);
	char cbuf[BUFSIZE * 2];
	vsprintf(cbuf, fmt, arg);
	va_end(arg);

	EnterCriticalSection(&cs);

	// 줄이 충분하지 않으면 빈 줄을 추가하여 line까지 늘림
	int current_line_count = SendMessage(hEdit, EM_GETLINECOUNT, 0, 0);
	if (line >= current_line_count) {
		for (int i = current_line_count; i <= line; i++) {
			SendMessage(hEdit, EM_SETSEL, -1, -1);
			SendMessageA(hEdit, EM_REPLACESEL, FALSE, (LPARAM)"\r\n");
		}
	}

	// line의 시작 인덱스를 가져와 해당 줄에 출력
	int start_index = SendMessage(hEdit, EM_LINEINDEX, line, 0);
	int end_index = SendMessage(hEdit, EM_LINEINDEX, line + 1, 0);
	if (end_index == -1) {
		end_index = GetWindowTextLength(hEdit); // 마지막 줄의 끝까지 선택
	}

	// 특정 줄을 선택하여 텍스트를 대체
	SendMessage(hEdit, EM_SETSEL, start_index, end_index);
	SendMessageA(hEdit, EM_REPLACESEL, FALSE, (LPARAM)cbuf);

	LeaveCriticalSection(&cs);
}

void UpdateProgressDisplay(int line, long int received_bytes, long int filesize)
{
	double percentage = ((double)received_bytes / filesize) * 100;

	// 1% 이상 차이 있을 때만 출력 갱신
	if ((int)percentage != (int)last_percentage) {
		last_percentage = percentage;
		DisplayText(line, "\r[TCP 서버] %ld 바이트를 수신했습니다. (%.2f%%)", received_bytes, percentage);
	}
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
	DisplayText(0, "[%s] %s\r\n", msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// 클라이언트와 데이터 통신 처리
DWORD WINAPI ProcessClient(LPVOID arg)
{
	SOCKET client_sock = (SOCKET)arg;
	struct sockaddr_in clientaddr;
	char addr[INET_ADDRSTRLEN];
	int addrlen;
	char buf[BUFSIZE + 1];
	long int filesize;
	long int received_bytes;
	int retval;
	FILE* file;

	// 클라이언트 정보 얻기
	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (struct sockaddr*)&clientaddr, &addrlen);
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

	// 스레드 번호
	static long thread_num;
	int thread_index = InterlockedIncrement(&thread_num) - 1; // 스레드 인덱스 >> 변수를 증가시켜줌. cpu특수 명령어라 동시접근 안돼서 좋음 

	EnterCriticalSection(&cs);
	DisplayText((thread_index * 10), "\r\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\r\n", addr, ntohs(clientaddr.sin_port));
	LeaveCriticalSection(&cs);

	// 클라이언트와 데이터 통신 (파일 수신)
	while (file_count < MAX_FILES) {
		// 데이터 받기(고정 길이 - 파일 크기)
		retval = recv(client_sock, (char*)&filesize, sizeof(long int), MSG_WAITALL);
		if (retval == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		else if (retval == 0)
			break;
		EnterCriticalSection(&cs);
		DisplayText(((thread_index * 10) + 2), "[TCP 서버] 파일 크기: %ld 바이트를 수신했습니다.\n", filesize);
		LeaveCriticalSection(&cs);

		// 파일 이름 생성
		char filename[32];
		snprintf(filename, sizeof(filename), "received_file_%d", file_count);
		file_count++; // 파일 번호 증가

		// 파일 열기
		file = fopen(filename, "wb");
		if (file == NULL) {
			printf("파일을 열 수 없습니다: %s\n", filename);
			closesocket(client_sock);
			return 1;
		}

		// 파일 데이터 받기
		received_bytes = 0;
		while (received_bytes < filesize) {
			retval = recv(client_sock, buf, BUFSIZE, 0);
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
				break;
			}
			else if (retval == 0)
				break;
			fwrite(buf, 1, retval, file);
			received_bytes += retval;

			// 전송률 출력
			EnterCriticalSection(&cs);
			UpdateProgressDisplay((thread_index * 10) + 4, received_bytes, filesize);
			LeaveCriticalSection(&cs);
		}

		// 파일 닫기
		fclose(file);
		EnterCriticalSection(&cs);
		DisplayText(((thread_index * 10) + 6), "\n[TCP 서버] 파일 전송 완료. 총 수신 바이트: %ld\n", received_bytes);
		LeaveCriticalSection(&cs);

		// 파일 개수 제한 초과 시 종료
		if (file_count >= MAX_FILES) {
			DisplayText(((thread_index * 10) + 6), "파일 수신 한도를 초과했습니다.\n");
			break;
		}
	}

	// 클라이언트 소켓 닫기
	closesocket(client_sock);
	EnterCriticalSection(&cs);
	DisplayText(((thread_index * 10) + 8), "[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n", addr, ntohs(clientaddr.sin_port));
	LeaveCriticalSection(&cs);

	return 0;
}

// TCP 서버 시작 부분
DWORD WINAPI ServerMain(LPVOID arg)
{
	int retval;
	InitializeCriticalSection(&cs);
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// 소켓 생성
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// 데이터 통신에 사용할 변수
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;
	HANDLE hThread;

	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			DisplayError("accept()");
			break;
		}

		// 접속한 클라이언트 정보 출력
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		
		// 스레드 생성
		hThread = CreateThread(NULL, 0, ProcessClient,
			(LPVOID)client_sock, 0, NULL);
		if (hThread == NULL) { closesocket(client_sock); }
		else { CloseHandle(hThread); }
	}

	// 소켓 닫기
	closesocket(listen_sock);
	DeleteCriticalSection(&cs);
	// 윈속 종료
	WSACleanup();
	return 0;
}
