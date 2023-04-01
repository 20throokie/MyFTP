#include "stdio.h"
#include "stdlib.h"
#include "winsock2.h"
#include "Windows.h"
#include <WS2tcpip.h>
#include "direct.h"
#include <inttypes.h>
#include <time.h>

#pragma comment(lib,"ws2_32.lib")

#define FILE_SIZE 1024
#define BUFSIZE 1040

struct Packet
{
	int seq;
	int ack;
	int end;
	char data[FILE_SIZE];
};
//struct Packet大小 1024+4*3=1036B
struct Feedback
{
	int ack;
	int rwnd;
};

int lget(SOCKET s, SOCKADDR_IN* client_addr, char* filename);

int main()
{
	WSAData wsd;           //初始化信息
	SOCKET s;

	// MTU以太网数据帧的长度在46-1500字节之间，1500：链路层的最大传输单元
	// 单个UDP传输的最大内容为1472字节（1500-20-8）
	// 网络中标准UDP值为576字节，最好在编程中将数据长度控制在548字节以内
	int nPort = 8180;      //设置本机服务的端口号
	SOCKADDR_IN siLocal{};    //远程发送机地址和本机接收机地址
	SOCKADDR_IN clientAddr{};
	int dwSendSize = sizeof(SOCKADDR);
	int dwRecvSize = sizeof(SOCKADDR_IN);
	int nRet;
	int temp = 1;

	//启动Winsock
	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
		printf("初始化网络失败\r\n");
		return 0;
	}
	else {
		printf("启动成功\r\n");
	}

	//创建socket
	s = socket(PF_INET, SOCK_DGRAM, 0);
	if (s == SOCKET_ERROR) {
		printf("创建套接字失败\r\n");
		closesocket(s);
		WSACleanup();
		return -2;
	}
	else {
		printf("创建套接字成功\r\n");
	}

	char on = 1;
	// 设置端口复用
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	//int nRecvBuf = 4 * 1024 * 1024;//设置为4M
	//setsockopt(s, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(nRecvBuf));

	siLocal.sin_family = AF_INET;
	siLocal.sin_port = htons(nPort);
	siLocal.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	//绑定本地地址到socket
	if (bind(s, (SOCKADDR*)&siLocal, sizeof(siLocal)) == SOCKET_ERROR) {
		printf("绑定套接字失败\r\n");
		closesocket(s);
		WSACleanup();
		return -3;
	}
	else {
		printf("绑定套接字成功\r\n");
	}

	char filename[MAX_PATH] = { 0 };
	// 开始接受文件名，并把客户端信息填入到clientAddr中
	nRet=recvfrom(s, filename, sizeof(filename), 0, (SOCKADDR*)&clientAddr, &dwRecvSize);
	if (nRet < 0)
	{
		printf("文件名接收错误\r\n");
		closesocket(s);
		WSACleanup();
		return -4;
	}
	sendto(s, (char*)&temp, sizeof(int), 0, (SOCKADDR*)&clientAddr, sizeof(SOCKADDR));

	//设置非阻塞模式
	int imode = 1;
	int rev = ioctlsocket(s, FIONBIO, (u_long*)&imode);
	if (rev == SOCKET_ERROR)
	{
		printf("设置非阻塞模式失败\r\n");
		closesocket(s);
		WSACleanup();
		return -1;
	}
	//设置socket的缓冲区
	int nSendBuf = 1024 * 64;
	int nRecvBuf = 1024 * 64;
	setsockopt(s, SOL_SOCKET, SO_SNDBUF, (const char*)&nSendBuf, sizeof(int));
	setsockopt(s, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(int));


	lget(s, &clientAddr, filename);
	closesocket(s);
	WSACleanup();
	system("pause");
	return 0;
}

int lget(SOCKET s, SOCKADDR_IN* client_addr, char* filename)
{
	FILE* fp2;
	fp2 = fopen(filename, "wb");
	int packet_count = 1;
	struct Packet Recv = { 0 };
	struct Feedback Send = { 0 };
	Send.rwnd = 110;
	struct Packet List = { 0 };
	SOCKADDR_IN server_addr{};
	int dwSendSize = sizeof(SOCKADDR);
	int dwRecvSize = sizeof(SOCKADDR_IN);
	int nRet;
	int temp;
	int bytes;
	int random;

	while (true)
	{
		memset(&Recv, 0, sizeof(Recv));
		nRet = recvfrom(s, (char*)&Recv, sizeof(Recv), 0, (SOCKADDR*)&server_addr, &dwRecvSize);
		if (nRet == SOCKET_ERROR || nRet == 0) {
			continue;
		}

		//设置随机丢包，并通知客户端要求重发
		srand((unsigned)time(NULL));
		random = (rand() % 100) + 1;
		if (random == 2)
		{
			printf("服务端已丢失第%d个包,要求客户端重发\r\n", Recv.seq);
			//反馈上一个包的ack
			Send.ack = Recv.ack - 1;
			sendto(s, (char*)&Send, sizeof(Send), 0, (SOCKADDR*)&server_addr, sizeof(SOCKADDR));
			continue;
		}
		
		packet_count += 1;
		if (Send.rwnd > 0)
		{
			if (Recv.seq == 0)
			{
				Send.ack = 0;
				sendto(s, (char*)&Send, sizeof(Send), 0, (SOCKADDR*)&server_addr, sizeof(SOCKADDR));
				continue;
			}
			//要求序号要连续，否则将该包直接丢弃，等待下一个序号包的到来
			if (Recv.ack != packet_count - 1)
			{
				printf("服务端接收第%d个包的序号不正确,要求服务器重发", Recv.seq);
				Send.ack = Recv.ack - 1;
				sendto(s, (char*)&Send, sizeof(Send), 0, (SOCKADDR*)&server_addr, sizeof(SOCKADDR));
				continue;
			}
			List.seq = Recv.seq;
			List.ack = Recv.ack;
			List.end = Recv.end;
			memcpy(List.data, Recv.data,sizeof(Recv.data));
			Send.rwnd -= 1;
			//接收完毕，发送ACK反馈包
			Send.ack = Recv.ack;
			bytes=sendto(s, (char*)&Send, sizeof(Send), 0, (SOCKADDR*)&server_addr, sizeof(SOCKADDR));
			if (bytes == SOCKET_ERROR || bytes == 0)
			{
				printf("%d\r\n", WSAGetLastError());
			}
		}
		else
		{
			Send.ack = Recv.ack;
			bytes = sendto(s, (char*)&Send, sizeof(Send), 0, (SOCKADDR*)&server_addr, sizeof(SOCKADDR));
		}
		
		fwrite(&List.data, nRet - sizeof(int)* 3, 1, fp2);
		printf("服务器已接收第%d个包，rwnd为%d\r\n", Recv.ack, Send.rwnd);
		temp = Recv.end;
		Send.rwnd += 1;
		memset(&List, 0, sizeof(List));
		if (temp == 1) break;
	}

	printf("文件接收完成，一共收了%d个包\r\n", packet_count);
	fclose(fp2);
	return 0;
}


