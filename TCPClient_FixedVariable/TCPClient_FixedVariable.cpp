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

char* SERVERIP = (char*)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    50

int main(int argc, char* argv[])
{
	int retval;

	// 명령행 인수 확인
	if (argc < 2) {
		printf("Usage: %s <file_name> <server_ip> \n", argv[0]);
		return 1;
	}
	const char* filename = argv[1];
	if (argc > 2) SERVERIP = argv[2];

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// 소켓 생성
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// connect()
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
		printf("파일을 열 수 없습니다: %s\n", filename);
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	// 파일 크기 구하기
	fseek(file, 0, SEEK_END);  // 파일 끝으로 이동
	long int filesize = ftell(file);  // 현재 파일 포인터 위치(파일 크기)
	fseek(file, 0, SEEK_SET);  // 파일 포인터를 다시 파일 시작으로 이동
	printf("파일 크기: %ld 바이트\n", filesize);
	if (filesize < 0) {
		printf("파일 크기를 가져올 수 없습니다: %s\n", filename);
		fclose(file);
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	// 파일 크기를 먼저 전송 (고정 길이 전송)
	retval = send(sock, (char*)&filesize, sizeof(long int), 0);
	if (retval == SOCKET_ERROR) {
		err_display("send()");
		fclose(file);
		closesocket(sock);
		WSACleanup();
		return 1;
	}
	printf("[TCP 클라이언트] 파일 크기 %ld 바이트를 보냈습니다.\n", filesize);

	// 데이터 통신에 사용할 변수
	char buf[BUFSIZE];
	size_t len;
	long int total_sent = 0;

	// 서버와 데이터 통신
	while ((len = fread(buf, 1, BUFSIZE, file)) > 0) {
        retval = send(sock, buf, (int)len, 0);
        if (retval == SOCKET_ERROR) {
            err_display("send()");
            break;
        }
		total_sent += retval;
    }
	printf("[TCP 클라이언트] 총 %ld 바이트를 보냈습니다.\n", total_sent);

	// 파일 닫기
	fclose(file);

	// 파일 전송 후 대기 상태
	while (1) {
		char user_input[BUFSIZE];
		printf("명령어를 입력하세요 (종료하려면 'exit'): ");
		fgets(user_input, BUFSIZE, stdin);  // 사용자 입력 받기

		if (strncmp(user_input, "exit", 4) == 0) {
			printf("프로그램을 종료합니다.\n");
			break;
		}
	}

	// 소켓 닫기
	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}
