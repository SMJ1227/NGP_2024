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

#define SERVERPORT 9000
#define BUFSIZE    512

int main(int argc, char* argv[])
{
	int retval;

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// ���� ����
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

	// ������ ��ſ� ����� ����
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;
	long int filesize; // ���� ���� ������
	char buf[BUFSIZE + 1]; // ���� ���� ������
	FILE* file;

	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		// ������ Ŭ���̾�Ʈ ���� ���
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", addr, ntohs(clientaddr.sin_port));

		// Ŭ���̾�Ʈ�� ������ ���
		while (1) {
			// ������ �ޱ�(���� ����)
			retval = recv(client_sock, (char*)&filesize, sizeof(long int), MSG_WAITALL);	//-- ���� ���� �����ʹ� WAITALL�� ���� ���������� return���� �ʾƾ���.
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
				break;
			}
			else if (retval == 0)
				break;
			printf("[TCP ����] ���� ũ��: %ld ����Ʈ�� �����߽��ϴ�.\n", filesize);

			// ���� ���� (���� ������ ������ ����)
			file = fopen("received_file", "wb");
			if (file == NULL) {
				printf("������ �� �� �����ϴ�.\n");
				closesocket(client_sock);
				continue;
			}

			// ���� ������ �ޱ� (���� ����)
			long int received_bytes = 0;
			while (received_bytes < filesize) {
				retval = recv(client_sock, buf, BUFSIZE, 0);	//-- ���� ���� �����ʹ� while�� filesize��ŭ �޵��� �߱� ������ WAILALL�� ������� �ʾƵ� ������.
				if (retval == SOCKET_ERROR) {
					err_display("recv()");
					break;
				}
				else if (retval == 0) {
					break;
				}
				fwrite(buf, 1, retval, file); // ���� �����͸� ���Ͽ� ����
				received_bytes += retval;
				double percentage = ((double)received_bytes / filesize) * 100;
				printf("[TCP ����] %d ����Ʈ�� �����߽��ϴ�. (%.2f%%)\n", retval, percentage);
			}

			// ���� �ݱ�
			fclose(file);
			printf("[TCP ����] ���� ���� �Ϸ�. �� ���� ����Ʈ: %ld\n", received_bytes);

			// ���� �ݱ�
			closesocket(client_sock);
			printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", addr, ntohs(clientaddr.sin_port));
		}

		// ���� �ݱ�
		closesocket(listen_sock);

		// ���� ����
		WSACleanup();
		return 0;
	}
}
