#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <string.h>
#include <mswsock.h>
#pragma comment(lib,"Ws2_32.lib")
#pragma comment(lib,"Mswsock.lib")

#define MAX_COUNT 1024
#define MAX_RECV_COUNT 1024

int SocketPrepore(void);//准备工作
int SocketError(SOCKET sock);//错误查询
int PostRecv(int iIndex);//投递收
int PostAccept(void);//投递链接
void Clear(void);//
int PostSend(int iIndex);//投递发
DWORD WINAPI ThreadProc(LPVOID lpParameter);//回调函数

char g_strbuf[1024] = { 0 };//WSARecv接收缓冲区
char g_name[20][20] = { 0 };//名字储存
int i = 1;
SOCKET g_allsock[MAX_COUNT];//句柄数组(包含服务器句柄）
OVERLAPPED g_alllap[MAX_COUNT];//事件数组
int g_count;//句柄与事件数量  !!句柄与事件必须一一对应!!!!
HANDLE hPort;//端口
int nProcessorsCount;//线程数量(系统核数量)
HANDLE* pThread;
//BOOL用于控制ThreadProc循环
BOOL g_flag = TRUE;

BOOL WINAPI fun(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_CLOSE_EVENT://窗口关闭进入此
		g_flag = FALSE;
		//关闭句柄
		for (int i = 0; i < nProcessorsCount; i++)
		{
			CloseHandle(pThread[i]);
		}
		free(pThread);
		//释放所有socket重叠io和hPort
		Clear();
		//清理网络库
		WSACleanup();
	}
	return TRUE;
}
int main(void)
{
	SetConsoleCtrlHandler(fun, TRUE);//事件钩子
	SOCKET _sockServer = (SOCKET)SocketPrepore();//准备工作(打开网络库....)
	if (0 == _sockServer)
	{
		//error
		printf("SocketPrepore Error\n");
		return 0;
	}
	if (0 != PostAccept())
	{
		//error
		printf("PostAccept Error\n");
		return 0;
	}
	//创建线程数量
	SYSTEM_INFO systemProcessorsCount;
	GetSystemInfo(&systemProcessorsCount);//获取系统核数量,可自我查询手动输入
	nProcessorsCount = systemProcessorsCount.dwNumberOfProcessors;
	pThread = (HANDLE*)malloc(sizeof(HANDLE) * nProcessorsCount);
	//创建线程句柄
	for (int i = 0; i < nProcessorsCount; i++)
	{
		pThread[i] = CreateThread(NULL, 0, ThreadProc, hPort, 0, NULL);
		if (NULL == pThread[i])
		{
			int a = GetLastError();
			printf("CreateThread Error:%d", a);
			CloseHandle(hPort);
			return SocketError(_sockServer);
		}
	}
	while (1)
	{
		//这里可进行任意代码操作，并不会被上一步阻塞
		Sleep(1000);
	}
	//删除线程句柄
	for (int i = 0; i < nProcessorsCount; i++)
	{
		CloseHandle(pThread[i]);
	}
	free(pThread);
	Clear();
	WSACleanup();
	system("pause");
	return 0;
}
int SocketPrepore(void)
{
	//打开
	WORD wdVersion = MAKEWORD(2, 2);
	WSADATA wdsockMsg;
	int nRes = WSAStartup(wdVersion, &wdsockMsg);
	if (0 != nRes)
	{
		switch (nRes)
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
	if (2 != HIBYTE(wdsockMsg.wVersion) || 2 != LOBYTE(wdsockMsg.wVersion))
	{
		WSACleanup();
		return 0;
	}
	//服务器socket
	SOCKET sockServer = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == sockServer)
	{
		return SocketError(sockServer);
	}
	//结构体
	struct sockaddr_in si = { 0 };
	si.sin_family = AF_INET;
	si.sin_port = htons(12349);//注意不要占用系统已经使用的端口号，可查询
	si.sin_addr.S_un.S_addr = inet_addr("172.18.9.165");//172.18.9.165
	if (SOCKET_ERROR == bind(sockServer, (const struct sockaddr*)&si, sizeof(si)))
	{
		return SocketError(sockServer);
	}
	g_allsock[g_count] = sockServer;//装入数组
	g_alllap[g_count].hEvent = WSACreateEvent();
	g_count++;
	//创建端口
	hPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (0 == hPort)
	{
		int a = GetLastError();
		printf("%d", a);
		closesocket(sockServer);
		WSACleanup();
		return 0;
	}
	//绑定
	HANDLE hPort1 = CreateIoCompletionPort((HANDLE)sockServer, hPort, 0, 0);
	if (hPort1 != hPort)
	{
		int a = GetLastError();
		printf("%d", a);
		CloseHandle(hPort);
		closesocket(sockServer);
		WSACleanup();
		return 0;
	}
	if (SOCKET_ERROR == listen(sockServer, 20))
	{
		return SocketError(sockServer);
	}
	return sockServer;
}
int SocketError(SOCKET sock)
{
	closesocket(sock);
	WSACleanup();
	return 0;
}
int PostAccept()
{
	//发生客户端链接请求，则走此函数
	g_allsock[g_count] = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_alllap[g_count].hEvent = WSACreateEvent();
	char str[1024] = { 0 };
	DWORD dwRecvcount;
	BOOL bRes = AcceptEx(g_allsock[0], g_allsock[g_count], str, 0, sizeof(struct sockaddr_in) + 16,
		sizeof(struct sockaddr_in) + 16, &dwRecvcount, &g_alllap[0]);
	int a = WSAGetLastError();
	if (ERROR_IO_PENDING != a)
	{
		//函数出错
		return 1;
	}
	return 0;
}
int PostRecv(int iIndex)
{
	WSABUF wsabuf = { 0 };
	wsabuf.buf = g_strbuf;
	wsabuf.len = MAX_RECV_COUNT;
	DWORD dwRecvCount;
	DWORD flags = 0;
	int nRes = WSARecv(g_allsock[iIndex], &wsabuf, 1, &dwRecvCount, &flags, &g_alllap[iIndex], NULL);
	int a = WSAGetLastError();
	if (ERROR_IO_PENDING != a)
	{
		//函数出错
		return 1;
	}
	return 0;
}
int PostSend(int iIndex)
{
	WSABUF wsabuf = { 0 };
	wsabuf.buf = g_strbuf;
	wsabuf.len = MAX_RECV_COUNT;
	DWORD  dwBuffcount;
	DWORD dwFlags = 0;
	int nRes = WSASend(g_allsock[iIndex], &wsabuf, 1, &dwBuffcount, dwFlags, &g_alllap[iIndex], NULL);
	int a = WSAGetLastError();
	if (ERROR_IO_PENDING != a)
	{
		//函数执行出错
		return 1;
	}
	return 0;
}
void Clear(void)
{
	for (int i = 0; i < g_count; i++)
	{
		if (g_allsock[i] == 0)
			continue;
		closesocket(g_allsock[i]);
		WSACloseEvent(g_alllap[i].hEvent);

	}
	CloseHandle(hPort);
}
DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	HANDLE port = (HANDLE)lpParameter;
	DWORD NumberOfBytesTransferred;
	ULONG_PTR index;
	LPOVERLAPPED lpOverlapped;
	while (g_flag)
	{
		BOOL bFlag = GetQueuedCompletionStatus(port, &NumberOfBytesTransferred, &index, &lpOverlapped, INFINITE);
		if (FALSE == bFlag)
		{
			int a = GetLastError();
			if (64 == a)
				printf("Client Force Close\n");
			continue;
		}
		//处理信号
		//accept
		if (0 == index)
		{
			printf("accept success\n");
			HANDLE hPort1 = CreateIoCompletionPort((HANDLE)g_allsock[g_count], hPort, g_count, 0);
			if (hPort1 != hPort)
			{
				int a = GetLastError();
				printf("accept error:%d", a);
				closesocket(g_allsock[g_count]);
				continue;
			}
			//新客户端投递recv
			PostRecv(g_count);
			g_count++;
			PostAccept(g_count);
		}
		else//可读可写
		{
			if (0 == NumberOfBytesTransferred)
			{
				//client下线
				printf("Client Close\n");
				closesocket(g_allsock[index]);
				if (g_alllap[index].hEvent == 0)
					continue;//只是为了解决警告加的循环
				WSACloseEvent(g_alllap[index].hEvent);
				g_allsock[index] = 0;
				g_alllap[index].hEvent = NULL;
			}
			else if (0 != g_strbuf[0])
			{
				//recv
				if (g_strbuf[0] == '&')//判断名字
				{
					strcpy(g_name[i], g_strbuf);//存入名字数组
					i++;
					memset(g_strbuf, 0, sizeof(g_strbuf));
					PostRecv(index);
					continue;
				}
				for (int i = 0; i < g_count; i++)//给每个客户端发送
				{
					if (i == index)
						continue;
					send(g_allsock[i], g_name[index], 20, 0);//用户名
					send(g_allsock[i], g_strbuf, 1024, 0);//内容
					printf("%s\n", g_strbuf);
				}
				memset(g_strbuf, 0, sizeof(g_strbuf));
				PostRecv(index);
			}
			else
			{
				//send
				//PostSend(index);
			}
		}
	}
	return 0;
}