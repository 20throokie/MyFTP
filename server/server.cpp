#include "stdio.h"
#include "stdlib.h"
#include "winsock2.h"
#include "Windows.h"
#include "direct.h"
#include <inttypes.h>
#include <process.h>

#pragma comment(lib,"ws2_32.lib")

#define SERVERIP "127.0.0.1"           //本机的环回地址，可以设置自己的IP地址
#define SERVERPORT 9190
#define PERRECV 4096
#define BUFSIZE 4096
#define PERSEND 4096

int RecvFile(SOCKET sock,char* path);
int DirDisPlay(SOCKET sock);
int ListDirectory(SOCKET sock, char* Path, int Recursive);
int SendFile(SOCKET sock, char filename[MAX_PATH], char path[MAX_PATH], int lastfold);
int ListDirectory2(SOCKET sock, char* Path, int Recursive);

unsigned WINAPI ThreadDirDisplay(void* arg);
unsigned WINAPI ThreadRecvFile(void* arg);
unsigned WINAPI ThreadSendFile(void* arg);

int countcheck = 0;
static int clntCnt = 0;
static int readcount = 0;
SOCKET clntSocks[256];
static HANDLE hMutex;
static HANDLE rw;

int main(void)
{
	int ret, len;
	WSADATA data;
	SOCKET sockListen, sockTx;
	HANDLE hThread;
	struct sockaddr_in addrServer,addrClient;
	ret = WSAStartup(MAKEWORD(2, 2), &data);
	if (SOCKET_ERROR == ret)
	{
		return -1;
	}

	sockListen = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == sockListen)
	{
		WSACleanup();
		return -2;
	}

	memset(&addrServer, 0, sizeof(struct sockaddr_in));
	addrServer.sin_family = AF_INET;
	addrServer.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addrServer.sin_port = htons(SERVERPORT);

	len = sizeof(struct sockaddr);
	ret = bind(sockListen, (struct sockaddr*)&addrServer, len);
	if (SOCKET_ERROR == ret)
	{
		closesocket(sockListen);
		WSACleanup();
		return -3;
	}

	ret = listen(sockListen, 5);
	if (SOCKET_ERROR == ret)
	{
		closesocket(sockListen);
		WSACleanup();
		return -4;
	}

	hMutex = CreateSemaphore(NULL, 1, 3, NULL);
	rw = CreateSemaphore(NULL, 1, 1, NULL);
	while (1)
	{
		len = sizeof(struct sockaddr);
		sockTx = accept(sockListen, (sockaddr*)&addrClient, &len);
		if (INVALID_SOCKET == sockTx)
		{
			closesocket(sockListen);
			WSACleanup();
			return -5;
		}

		printf("TCP连接成功...\r\n");

		//设置I/O缓冲区大小
		int Send = 1024 * 32, Recv = 1024 * 32;
		ret = setsockopt(sockTx, SOL_SOCKET, SO_SNDBUF, (char*)&Send, sizeof(Send));
		ret = setsockopt(sockTx, SOL_SOCKET, SO_RCVBUF, (char*)&Recv, sizeof(Recv));

		char recvBuf[BUFSIZE], sendBuf[BUFSIZE];
		memset(recvBuf, 0, sizeof(recvBuf));
		memset(sendBuf, 0, sizeof(sendBuf));

		TCHAR Buffer[MAX_PATH];
		DWORD dwRet;
		dwRet = GetCurrentDirectory(MAX_PATH, Buffer);
		
		recv(sockTx, recvBuf, 1, 0);
		if (recvBuf[0] == 1)
		{
			hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadDirDisplay, (void*)&sockTx, 0, NULL);
			//DirDisPlay(sockTx);
			//closesocket(sockListen);
			//closesocket(sockTx);
			//WSACleanup();
		}
		else if (recvBuf[0] == 2)
		{
			//ret=RecvFile(sockTx,Buffer);
			hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadRecvFile, (void*)&sockTx, 0, NULL);
		}
		else if (recvBuf[0] == 3)
		{
			memset(recvBuf, 0, sizeof(recvBuf));
			recv(sockTx, recvBuf, sizeof(recvBuf), 0);
			printf("%s\r\n", recvBuf);
			closesocket(sockTx);
		}
		else if (recvBuf[0] == 4)
		{
			hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadSendFile, (void*)&sockTx, 0, NULL);
		}
		else
		{
			printf("未知错误\r\n");
			closesocket(sockTx);
			break;
		}
	}
	closesocket(sockListen);
	WSACleanup();
	system("pause");
	return 0;
}

int RecvFile(SOCKET sock,char* path)
{
	//printf("等待接收文件...\r\n");
	int ret, lastsend, curRecv;
	__int64 totlen,totRecv;
	FILE *fp;
	char sendBuf[BUFSIZE], recvBuf[BUFSIZE];
	char temp[MAX_PATH] = { 0 };

	if (countcheck == 0)
	{
		memset(recvBuf, 0, sizeof(recvBuf));
		recv(sock, recvBuf, sizeof(recvBuf), 0);
		strcpy(temp, recvBuf);
		strcat(temp, "\\");
		countcheck++;
	}
	else
	{
		strcpy(temp, path);
		strcat(temp, "\\");
	}

	memset(recvBuf, 0, sizeof(recvBuf));
	recv(sock, recvBuf, 1, 0);
	//printf("%d\r\n", (int)recvBuf[0]);

	if (recvBuf[0] == 3)
	{
		memset(recvBuf, 0, sizeof(recvBuf));
		recv(sock, recvBuf, sizeof(recvBuf), 0);
		strncat(temp, recvBuf, strlen(recvBuf) + 1); 
		printf("创建并进入文件夹: %s\r\n",recvBuf);
		memset(sendBuf, 0, sizeof(sendBuf));
		sendBuf[0] = 1;
		send(sock, sendBuf, 1, 0);
		_mkdir(temp);
		//printf("%s\r\n", temp);
		while (1)
		{
			ret=RecvFile(sock, temp);
			if (ret == -6)
			{
				break;
			}
			else if (ret == -7)
			{
				printf("关闭并退出文件夹: %s\r\n", recvBuf);
				//printf("end recv\r\n");
				return 0;
			}
			else if (ret != 0)
			{
				return ret;
			}
		}
		return 0;
	}
	else if (recvBuf[0] == 4)
	{
		return -6;
	}
	else if (recvBuf[0] == 5)
	{
		return -7;
	}
	else if (recvBuf[0] == 6)
	{
		printf("上传的文件名错误\r\n");
		return -8;
	}
	else if (recvBuf[0] != 2)
	{
		printf("传输中遇到错误\r\n");
		return -9;
	}

	memset(recvBuf, 0, sizeof(recvBuf));
	ret = recv(sock, recvBuf, sizeof(recvBuf), 0);
	if (SOCKET_ERROR == ret||ret==0)
	{
		return -1;
	}

	printf("接收到客户端发来的文件名: %s\n", recvBuf);

	//memset(sendBuf, 0, sizeof(sendBuf));
	//strcpy(sendBuf, recvBuf);
	//ret = send(sock, sendBuf, strlen(sendBuf), 0);
	//if (SOCKET_ERROR == ret)
	//{
	//	return -2;
	//}


	//printf("请输入你希望保存的文件名（带扩展名）\n");
	strncat(temp, recvBuf, strlen(recvBuf) + 1);
//	scanf("%s", filename);
//	char message[30];
//	ret = recv(sock, message, sizeof(message)-1, 0);
	fp = fopen(temp, "wb");
	if (NULL == fp)
	{
		return -3;
	}

	memset(recvBuf, 0, sizeof(recvBuf));
	ret = recv(sock, recvBuf, sizeof(recvBuf), 0);
	if (SOCKET_ERROR == ret)
	{
		return -4;
	}

	printf("成功接收客户端发来的文件长度: %s\n", recvBuf);

	totlen = _atoi64(recvBuf);
	//printf("%" PRId64 "\r\n", totlen);

	memset(recvBuf, 0, sizeof(recvBuf));
	ret = recv(sock, recvBuf, sizeof(recvBuf), 0);
	if (SOCKET_ERROR == ret)
	{
		return -4;
	}
	printf("最后一次要发送的字节: %s\r\n", recvBuf);
	lastsend = atoi(recvBuf);
	//memset(sendBuf, 0, sizeof(sendBuf));
	//strcpy(sendBuf, recvBuf);
	//ret = send(sock, sendBuf, strlen(sendBuf), 0);
	//if (SOCKET_ERROR == ret)
	//{
	//	return -5;
	//}

	totRecv = 0;
	//循环接收,直到接收完毕(totrecv==totlen)
	while (totRecv + PERRECV<=totlen)
	{
		memset(recvBuf, 0, sizeof(recvBuf));
		curRecv = recv(sock, recvBuf, PERRECV, 0);
		if (SOCKET_ERROR == curRecv){ fclose(fp); return -2; }

		totRecv = totRecv + curRecv;
		printf("总大小%" PRId64 "字节	", totlen);
		printf("已接收%" PRId64 "字节    ", totRecv);
		printf("进度 %.2lf""%%\r\n", 100*double(totRecv)/double(totlen));
		ret = fwrite(recvBuf, curRecv, 1, fp);
		if (ret != 1){ fclose(fp); return -2; }
		if (totRecv >= totlen) break;
	}
	memset(recvBuf, 0, sizeof(recvBuf));
	curRecv = recv(sock, recvBuf, lastsend, 0);
	if (SOCKET_ERROR == curRecv){ fclose(fp); return -2; }
	ret = fwrite(recvBuf, curRecv, 1, fp);
	if (ret != 1){ fclose(fp); return -2; }


	//确认
	memset(sendBuf, 0, sizeof(sendBuf));
	sendBuf[0] = 2;
	send(sock, sendBuf, 1, 0);
	fclose(fp);
	printf("成功接收一个文件\r\n");
	//printf("end recv\r\n");
	return 0;
}

int DirDisPlay(SOCKET sock)
{
	char recvBuf[BUFSIZE], sendBuf[BUFSIZE];
	TCHAR Buffer[MAX_PATH];
	DWORD dwRet;
	int ret;

	memset(recvBuf, 0, sizeof(recvBuf));
	recv(sock, recvBuf, 1, 0);
	if (recvBuf[0] == 0)
	{
		dwRet = GetCurrentDirectory(MAX_PATH, Buffer);
	}
	else if (recvBuf[0] == 1)
	{
		memset(recvBuf, 0, sizeof(recvBuf));
		recv(sock, recvBuf, sizeof(recvBuf), 0);
		memset(Buffer, 0, sizeof(Buffer));
		strcpy(Buffer, recvBuf);
	}
	
	printf("开始查找目录\n");

	ret=ListDirectory(sock, Buffer, 0);
	if (ret == 0)
		printf("获取目录成功\r\n");
	else
		printf("获取目录失败\r\n");

	return 0;
}

int ListDirectory(SOCKET sock, char* Path, int Recursive)
{
	HANDLE hFind;
	WIN32_FIND_DATA FindFileData;
	char filename[MAX_PATH] = { 0 };
	char sendBuf[BUFSIZE] = { 0 };
	int Ret = 0;

	strcpy(filename, Path);
	strcat(filename, "\\");
	strcat(filename, "*.*");

	// 查找第一个文件
	hFind = FindFirstFile(filename, &FindFileData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		//printf("Error when list %s\n", Path);
		memset(sendBuf, 0, sizeof(sendBuf));
		sendBuf[0] = -2;
		send(sock, sendBuf, 1, 0);
		return -1;
	}
	//if (!SetCurrentDirectory(Path))
	//{
	//	memset(sendBuf, 0, sizeof(sendBuf));
	//	sendBuf[0] = -2;
	//	send(sock, sendBuf, 1, 0);
	//	return -4;
	//}
	memset(sendBuf, 0, sizeof(sendBuf));
	sendBuf[0] = 0;
	send(sock, sendBuf, 1, 0);
	
	do
	{
		// 构造文件名
		memset(filename, 0, sizeof(filename));
		strcpy(filename, Path);
		strcat(filename, "\\");
		strcat(filename, FindFileData.cFileName);
		//printf("%s\n", filename);
		//int len = 0;
		//while (*(filename + len) != 0)
		//{
		//	*FileName = *(filename + len);
		//	FileName++;
		//	len++;
		//}
		//strncat(FileName, "?", 1);
		//FileName = FileName++;
		memset(sendBuf, 0, sizeof(sendBuf));
		strcpy(sendBuf, filename);
		send(sock, sendBuf, sizeof(sendBuf),0);


		// 如果是递归查找，并且文件名不是.和..，并且文件是一个目录，那么执行递归操作
		if (Recursive != 0
			&& strcmp(FindFileData.cFileName, ".") != 0
			&& strcmp(FindFileData.cFileName, "..") != 0
			&& FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			Ret=ListDirectory(sock, filename, Recursive);
			if (Ret != 0) return -2;
		}
		// 查找下一个文件
		if (FindNextFile(hFind, &FindFileData) == FALSE)
		{
			// ERROR_NO_MORE_FILES 表示已经全部查找完成
			if (GetLastError() != ERROR_NO_MORE_FILES)
			{
				//printf("Error when get next file in %s\n", Path);
				return -3;
			}
			else
			{
				memset(sendBuf, 0, sizeof(sendBuf));
				sendBuf[0] = -1;
				send(sock, sendBuf, sizeof(sendBuf), 0);
				Ret = 0;
			}
			break;
		}
	} while (TRUE);


	// 关闭句柄
	FindClose(hFind);
	return Ret;
}

int SendFile(SOCKET sock, char filename[MAX_PATH], char path[MAX_PATH], int lastfold)
{
	int ret, temp3;
	char sendBuf[BUFSIZE], recvBuf[BUFSIZE];
	FILE *fp;
	int i, lastsend;
	__int64 totlen, cnt,CurSend;

	char temp[MAX_PATH] = { 0 };
	char temp4[MAX_PATH] = { 0 };
	strcpy(temp, path);
	strcpy(temp4, path);
	strcat(temp4, "\\");
	strcat(temp4, filename);

	fp = fopen(temp4, "rb");
	if (NULL == fp)
	{
		if (-1 != GetFileAttributes(temp4))
		{
			memset(sendBuf, 0, sizeof(sendBuf));
			sendBuf[0] = 3;
			send(sock, sendBuf, 1, 0);
			memset(sendBuf, 0, sizeof(sendBuf));
			strcpy(sendBuf, filename);
			send(sock, sendBuf, strlen(sendBuf) + 1, 0);
			Sleep(10);
			memset(recvBuf, 0, sizeof(recvBuf));
			recv(sock, recvBuf, 1, 0);
			if (recvBuf[0] != 1) return -7;
			temp3 = ListDirectory2(sock, temp4, 1);
			if (temp3 != 0) return -8;
			if (lastfold == 1)
			{
				memset(sendBuf, 0, sizeof(sendBuf));
				sendBuf[0] = 4;
				send(sock, sendBuf, 1, 0);
				Sleep(10);
			}
			else if (lastfold == 0)
			{
				memset(sendBuf, 0, sizeof(sendBuf));
				sendBuf[0] = 5;
				send(sock, sendBuf, 1, 0);
				Sleep(10);
			}
			return 0;
		}
		else
		{
			memset(sendBuf, 0, sizeof(sendBuf));
			sendBuf[0] = 6;
			send(sock, sendBuf, 1, 0);
			Sleep(10);
			return -1;
		}
	}

	memset(sendBuf, 0, sizeof(sendBuf));
	sendBuf[0] = 2;
	send(sock, sendBuf, 1, 0);


	_fseeki64(fp, 0, SEEK_END);
	totlen = _ftelli64(fp);

	memset(sendBuf, 0, sizeof(sendBuf));
	strcpy(sendBuf, filename);

	ret = send(sock, sendBuf, sizeof(sendBuf), 0);
	if (SOCKET_ERROR == ret)
	{
		return -2;
	}

	//memset(recvBuf, 0, sizeof(recvBuf));
	//ret = recv(sock, recvBuf, sizeof(recvBuf), 0);
	//if (SOCKET_ERROR == ret || (strcmp(sendBuf, recvBuf) != 0))
	//{
	//	return -3;
	//}



	memset(sendBuf, 0, sizeof(sendBuf));
	_i64toa(totlen, sendBuf, 10);
	ret = send(sock, sendBuf, sizeof(sendBuf), 0);
	if (SOCKET_ERROR == ret)
	{
		return -3;
	}

	//memset(recvBuf, 0, sizeof(recvBuf));
	//ret = recv(sock, recvBuf, sizeof(recvBuf), 0);
	//if (SOCKET_ERROR == ret || (strcmp(sendBuf, recvBuf) != 0))
	//{
	//	return -5;
	//}

	//	printf("向服务器发送文件长度成功\n");

	lastsend = PERSEND;
	cnt = totlen / PERSEND;
	if (totlen%PERSEND)
	{
		cnt = cnt + 1;
		lastsend = totlen%PERSEND;
	}

	memset(sendBuf, 0, sizeof(sendBuf));
	itoa(lastsend, sendBuf, 10);
	ret = send(sock, sendBuf, sizeof(sendBuf), 0);
	if (SOCKET_ERROR == ret)
	{
		return -4;
	}
	Sleep(10);

	fp = fopen(temp4, "rb");
	if (NULL == fp)
	{
		return -1;
	}

	CurSend = 0;
	printf("*******************************************************************\r\n");
	//前cnt-1每次传PERSEND字节
	for (i = 1; i <= cnt - 1; i++)
	{
		memset(sendBuf, 0, sizeof(sendBuf));
		ret = fread(sendBuf, PERSEND, 1, fp);
		if (ret != 1){ fclose(fp); return -5; }

		ret = send(sock, sendBuf, PERSEND, 0);
		if (ret != PERSEND){ fclose(fp); return -5; }
		CurSend = CurSend + PERSEND;
		printf("总大小%" PRId64 "字节	", totlen);
		printf("已发送%" PRId64 "字节    ", CurSend);
		printf("进度 %.2lf""%%\r\n", 100 * double(CurSend) / double(totlen));
	}


	//最后一次传 lastsend字节
	memset(sendBuf, 0, sizeof(sendBuf));
	ret = fread(sendBuf, lastsend, 1, fp);
	if (ret != 1){ fclose(fp); return -5; }
	ret = send(sock, sendBuf, lastsend, 0);
	if (ret != lastsend){ fclose(fp); return -5; }
	printf("*******************************************************************\r\n");

	memset(recvBuf, 0, sizeof(recvBuf));
	recv(sock, recvBuf, 1, 0);
	if (recvBuf[0] != 2) return -9;
	fclose(fp);
	printf("成功发送一个文件\r\n");
	return 0;
}

int ListDirectory2(SOCKET sock, char* Path, int Recursive)
{
	HANDLE hFind;
	WIN32_FIND_DATA FindFileData;
	char fname[MAX_PATH];
	int Ret = 0;
	int i = 0;

	//TCHAR temp[MAX_PATH];
	for (i = 0; i < MAX_PATH; i++)
	{
		fname[i] = *(Path + i);
	}
	//strcpy(fname, Path);
	strcat(fname, "\\");
	strcat(fname, "*.*");
	//CharToTchar(fname, temp);

	// 查找第一个文件
	hFind = FindFirstFile(fname, &FindFileData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		//printf("Error when list %s\n", Path);
		return -1;
	}
	do
	{
		// 构造文件名
		//char temp2[260] = { 0 };
		//wcstombs(temp2, FindFileData.cFileName, 260);
		strcpy(fname, Path);
		strcat(fname, "\\");
		strcat(fname, FindFileData.cFileName);
		//printf("%s\n", fname);
		if (strcmp(FindFileData.cFileName, ".") != 0 && strcmp(FindFileData.cFileName, "..") != 0)
		{
			Ret=SendFile(sock, FindFileData.cFileName, Path, 0);
			if (Ret != 0)
				return -2;
		}

		//// 如果是递归查找，并且文件名不是.和..，并且文件是一个目录，那么执行递归操作
		//if (Recursive != 0
		//	&& strcmp(temp2, ".") != 0
		//	&& strcmp(temp2, "..") != 0
		//	&& FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		//{
		//	ListDirectory(sock,fname, Recursive);
		//}
		// 查找下一个文件
		if (FindNextFile(hFind, &FindFileData) == FALSE)
		{
			// ERROR_NO_MORE_FILES 表示已经全部查找完成
			if (GetLastError() != ERROR_NO_MORE_FILES)
			{
				//printf("Error when get next file in %s\n", Path);
				return -3;
			}
			else
			{
				Ret = 0;
			}
			break;
		}
	} while (TRUE);

	// 关闭句柄
	FindClose(hFind);
	return Ret;
}

unsigned WINAPI ThreadDirDisplay(void* arg)
{
	SOCKET sock = *((SOCKET*)arg);

	WaitForSingleObject(hMutex, INFINITE);
	if (readcount == 0)
	{
		WaitForSingleObject(rw, INFINITE);
	}
	readcount++;
	ReleaseSemaphore(hMutex, 1, NULL);

	DirDisPlay(sock);

	WaitForSingleObject(hMutex, INFINITE);
	readcount--;
	if (readcount == 0)
	{
		ReleaseSemaphore(rw, 1, NULL);
	}
	ReleaseSemaphore(hMutex, 1, NULL);

	closesocket(sock);
	return 0;
}
unsigned WINAPI ThreadRecvFile(void* arg)
{
	int ret;
	SOCKET sockTx = *((SOCKET*)arg);
	WaitForSingleObject(rw, INFINITE);
	countcheck = 0;
	ret = RecvFile(sockTx, NULL);
	countcheck = 0;
	ReleaseSemaphore(rw,1,NULL);
	if (ret == 0)
		printf("文件接收成功\r\n");
	else
	{
		printf("文件接收失败\r\n");
		printf("返回值：%d\r\n", ret);
	}
	closesocket(sockTx);
	return 0;
}
unsigned WINAPI ThreadSendFile(void* arg)
{
	SOCKET sockTx=*((SOCKET*)arg);
	char filename[MAX_PATH] = { 0 };
	char recvBuf[BUFSIZE];
	int ret;
	memset(recvBuf, 0, sizeof(recvBuf));
	recv(sockTx, recvBuf, sizeof(recvBuf), 0);//接收客户端要求的文件名
	strcpy(filename, recvBuf);
	printf("接收到客户端请求的文件名: %s\r\n", recvBuf);
	memset(recvBuf, 0, sizeof(recvBuf));
	recv(sockTx, recvBuf, sizeof(recvBuf), 0);//接收客户端要求保存到的路径
	WaitForSingleObject(hMutex, INFINITE);
	if (readcount == 0)
	{
		WaitForSingleObject(rw, INFINITE);
	}
	readcount++;
	ReleaseSemaphore(hMutex,1,NULL);

	ret = SendFile(sockTx, filename, recvBuf, 1);

	WaitForSingleObject(hMutex, INFINITE);
	readcount--;
	if (readcount == 0)
	{
		ReleaseSemaphore(rw,1,NULL);
	}
	ReleaseSemaphore(hMutex,1,NULL);

	if (ret == 0)
	{
		printf("文件传输成功\r\n");
	}
	else
	{
		printf("文件传输失败\r\n");
		printf("返回值：%d\r\n", ret);
	}
	closesocket(sockTx);
	return 0;
}