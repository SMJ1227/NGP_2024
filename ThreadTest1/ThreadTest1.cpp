#define TestNumber 3

#if TestNumber == 1

#include <windows.h>
#include <stdio.h>

struct Point3D
{
	int x, y, z;
};

DWORD WINAPI MyThread(LPVOID arg)
{
	Sleep(1000);
	Point3D* pt = (Point3D*)arg;
	printf("Running MyThread() %d: %d, %d, %d\n",
		GetCurrentThreadId(), pt->x, pt->y, pt->z);
	return 0;
}

int main(int argc, char* argv[])
{
	// ù ��° ������ ����
	Point3D pt1 = { 10, 20, 30 };
	HANDLE hThread1 = CreateThread(NULL, 0, MyThread, &pt1, 0, NULL);
	if (hThread1 == NULL) return 1;
	CloseHandle(hThread1);

	// �� ��° ������ ����
	Point3D pt2 = { 40, 50, 60 };
	HANDLE hThread2 = CreateThread(NULL, 0, MyThread, &pt2, 0, NULL);
	if (hThread2 == NULL) return 1;
	CloseHandle(hThread2);

	printf("Running main() %d\n", GetCurrentThreadId());
	Sleep(2000);
	return 0;
}

#elif TestNumber == 2

#include <windows.h>
#include <stdio.h>

DWORD WINAPI MyThread(LPVOID arg)
{
	while (1);
	return 0;
}

int main()
{
	// �켱���� ���� ������ ����Ѵ�.
	printf("�켱����: %d ~ %d\n", THREAD_PRIORITY_IDLE,
		THREAD_PRIORITY_TIME_CRITICAL);

	// CPU ������ �˾Ƴ���.
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	printf("Num of CPU: %d\n", (int)si.dwNumberOfProcessors);

	// CPU ������ŭ �����带 �����Ѵ�.
	for (int i = 0; i < (int)si.dwNumberOfProcessors; i++) {
		// �����带 �����Ѵ�.
		HANDLE hThread = CreateThread(NULL, 0, MyThread, NULL, 0, NULL);
		// �켱������ ���� �����Ѵ�.
		SetThreadPriority(hThread, THREAD_PRIORITY_ABOVE_NORMAL);
		CloseHandle(hThread);
	}

	// �켱������ ���� �����Ѵ�.
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
	Sleep(1000);
	printf("�� ������ ����!\n");
	return 0;
}

#elif TestNumber == 3

#include <windows.h>
#include <stdio.h>

int sum = 0;

DWORD WINAPI MyThread(LPVOID arg)
{
	int num = (int)(long long)arg;
	for (int i = 1; i <= num; i++)
		sum += i;
	return 0;
}

int main(int argc, char* argv[])
{
	int num = 100 ;
	HANDLE hThread = CreateThread(NULL, 0, MyThread,
		(LPVOID)(long long)num, CREATE_SUSPENDED, NULL);

	printf("������ ���� ��. ��� ��� = %d\n", sum);
	ResumeThread(hThread);
	WaitForSingleObject(hThread, INFINITE);
	printf("������ ���� ��. ��� ��� = %d\n", sum);
	CloseHandle(hThread);
	return 0;
}

#endif