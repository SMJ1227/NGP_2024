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
	// 첫 번째 스레드 생성
	Point3D pt1 = { 10, 20, 30 };
	HANDLE hThread1 = CreateThread(NULL, 0, MyThread, &pt1, 0, NULL);
	if (hThread1 == NULL) return 1;
	CloseHandle(hThread1);

	// 두 번째 스레드 생성
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
	// 우선순위 값의 범위를 출력한다.
	printf("우선순위: %d ~ %d\n", THREAD_PRIORITY_IDLE,
		THREAD_PRIORITY_TIME_CRITICAL);

	// CPU 개수를 알아낸다.
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	printf("Num of CPU: %d\n", (int)si.dwNumberOfProcessors);

	// CPU 개수만큼 스레드를 생성한다.
	for (int i = 0; i < (int)si.dwNumberOfProcessors; i++) {
		// 스레드를 생성한다.
		HANDLE hThread = CreateThread(NULL, 0, MyThread, NULL, 0, NULL);
		// 우선순위를 높게 설정한다.
		SetThreadPriority(hThread, THREAD_PRIORITY_ABOVE_NORMAL);
		CloseHandle(hThread);
	}

	// 우선순위를 낮게 설정한다.
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
	Sleep(1000);
	printf("주 스레드 실행!\n");
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

	printf("스레드 실행 전. 계산 결과 = %d\n", sum);
	ResumeThread(hThread);
	WaitForSingleObject(hThread, INFINITE);
	printf("스레드 실행 후. 계산 결과 = %d\n", sum);
	CloseHandle(hThread);
	return 0;
}

#endif