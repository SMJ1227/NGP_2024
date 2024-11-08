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

char* SERVERIP = (char*)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    50

int main(int argc, char* argv[])
{
	int retval;

	// ����� �μ� Ȯ��
	if (argc < 2) {
		printf("Usage: %s <file_name> <server_ip> \n", argv[0]);
		return 1;
	}
	const char* filename = argv[1];
	if (argc > 2) SERVERIP = argv[2];

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// ���� ����
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
	
	// ���� ����
	FILE* file = fopen(filename, "rb");
	if (file == NULL) {
		printf("������ �� �� �����ϴ�: %s\n", filename);
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	// ���� ũ�� ���ϱ�
	fseek(file, 0, SEEK_END);  // ���� ������ �̵�
	long int filesize = ftell(file);  // ���� ���� ������ ��ġ(���� ũ��)
	fseek(file, 0, SEEK_SET);  // ���� �����͸� �ٽ� ���� �������� �̵�
	printf("���� ũ��: %ld ����Ʈ\n", filesize);
	if (filesize < 0) {
		printf("���� ũ�⸦ ������ �� �����ϴ�: %s\n", filename);
		fclose(file);
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	// ���� ũ�⸦ ���� ���� (���� ���� ����)
	retval = send(sock, (char*)&filesize, sizeof(long int), 0);
	if (retval == SOCKET_ERROR) {
		err_display("send()");
		fclose(file);
		closesocket(sock);
		WSACleanup();
		return 1;
	}
	printf("[TCP Ŭ���̾�Ʈ] ���� ũ�� %ld ����Ʈ�� ���½��ϴ�.\n", filesize);

	// ������ ��ſ� ����� ����
	char buf[BUFSIZE];
	size_t len;
	long int total_sent = 0;

	// ������ ������ ���
	while ((len = fread(buf, 1, BUFSIZE, file)) > 0) {
        retval = send(sock, buf, (int)len, 0);
        if (retval == SOCKET_ERROR) {
            err_display("send()");
            break;
        }
		total_sent += retval;
    }
	printf("[TCP Ŭ���̾�Ʈ] �� %ld ����Ʈ�� ���½��ϴ�.\n", total_sent);

	// ���� �ݱ�
	fclose(file);

	// ���� ���� �� ��� ����
	while (1) {
		char user_input[BUFSIZE];
		printf("��ɾ �Է��ϼ��� (�����Ϸ��� 'exit'): ");
		fgets(user_input, BUFSIZE, stdin);  // ����� �Է� �ޱ�

		if (strncmp(user_input, "exit", 4) == 0) {
			printf("���α׷��� �����մϴ�.\n");
			break;
		}
	}

	// ���� �ݱ�
	closesocket(sock);

	// ���� ����
	WSACleanup();
	return 0;
}
