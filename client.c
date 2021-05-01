#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")

DWORD WINAPI ThreadProc(LPVOID lpParameter);

char recvbuf[1500] = { 0 };//接受缓冲区
char buf[1500] = { 0 };//名字储存
char ip[30] = { 0 };//IP地址储存
SOCKET socketServer;
BOOL b_Flag = TRUE;//控制循环
HANDLE pThread = NULL;

BOOL WINAPI fun(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_CLOSE_EVENT:
		b_Flag = FALSE;
		//关闭句柄
			CloseHandle(pThread);
		free(pThread);
		//清理网络库
		closesocket(socketServer);
		WSACleanup();

	}
	return TRUE;
}
int main(void)
{
	//打开网络库
	WORD wdVersion = MAKEWORD(2, 2);
	WSADATA wdSockMsg;
	int nres = WSAStartup(wdVersion, &wdSockMsg);
	if (0 != nres)
	{
		switch (nres)
		{
		case WSASYSNOTREADY:
			printf("重启电脑或检查网络库");
			break;
		case WSAVERNOTSUPPORTED:
			printf("更新网络库");
			break;
		case WSAEINPROGRESS:
			printf("重启程序");
			break;
		case WSAEPROCLIM:
			printf("关闭其他后台软件");
			break;
		}
	}
	//校验版本
	if (2 != HIBYTE(wdSockMsg.wVersion) || 2 != LOBYTE(wdSockMsg.wVersion))
	{
		//清理网络库
		WSACleanup();
		return 0;
	}
	//服务器的socket
	socketServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	int a = WSAGetLastError();
	if (INVALID_SOCKET == socketServer)
	{
		//查询错误码
		int a = WSAGetLastError();
		//清理网络库
		WSACleanup();
		return 0;
	}
	printf("请输入你的IPV4地址：\n");
	scanf_s("%s",ip,29);
	printf("请输入你的昵称：(昵称格式以&开头，以：结尾）\n");
	scanf_s("%s", buf,1499);
	struct sockaddr_in si;
	si.sin_family = AF_INET;
	si.sin_port = htons(12349);
	si.sin_addr.S_un.S_addr = inet_addr(ip);
	if (SOCKET_ERROR == connect(socketServer, (struct sockaddr*)&si, sizeof(si)))
	{
 		int a = WSAGetLastError();
		//清理网络库
		closesocket(socketServer);
		WSACleanup();
		return 0;
	}
	pThread = (HANDLE*)malloc(sizeof(HANDLE));
	//创建线程句柄
	int i = 1;
		pThread = CreateThread(NULL, 0, ThreadProc, (LPVOID)i, 0, NULL);
		if (NULL == pThread)
		{
			int a = GetLastError();
			printf("CreateThread Error:%d", a);
			return 0;
		}
		send(socketServer, buf, strlen(buf), 0);
	while (1)
	{
		scanf_s("%s", buf,1499);
		if ('0' == buf[0])
		{
			break;
		}
		if(SOCKET_ERROR == send(socketServer, buf, strlen(buf), 0))
		{
			//出错
			int a = WSAGetLastError();
		}
		memset(buf, 0, 1024);
	}
	//清理网络库
	closesocket(socketServer);
	WSACleanup();

	system("pause");
	return 0;
}
DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	int i = 0;
	while (b_Flag)
	{
			if (SOCKET_ERROR == recv(socketServer, recvbuf, 1024, 0))
			{
				//error
				return 1;
			}
			if (0 != recvbuf[0])
			{
				
				printf("%s", recvbuf);
				i++;
				if (i == 2)
				{
					putchar('\n');
					i = 0;
				}
			}
			else
				continue;
			memset(recvbuf, 0, sizeof(recvbuf));
	}
	return 0;
}