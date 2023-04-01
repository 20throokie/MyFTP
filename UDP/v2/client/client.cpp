#include "stdio.h"
#include "stdlib.h"
#include "winsock2.h"
#include "Windows.h"
#include <WS2tcpip.h>
#include "direct.h"
#include <inttypes.h>
#include <string.h>
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

int lsend(SOCKET s, SOCKADDR_IN* server_addr, char* filename);

int main()
{
	WSAData wsd;           //初始化信息
	SOCKET s;         //发送到的目的SOCKET
	int nRet = 0;
	SOCKADDR_IN serverAddr{};    //服务器socket地址
	SOCKADDR_IN temp{};
	int dwSendSize = sizeof(SOCKADDR);
	int dwRecvSize = sizeof(SOCKADDR_IN);
	int filenamereceived=0;
	FILE* test;

	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
		printf("初始化网络失败\r\n");
		return 0;
	}
	else {
		printf("启动成功\r\n");
	}

	//创建socket

	//AF_INET 协议族:决定了要用ipv4地址（32位的）与端口号（16位的）的组合
	//SOCK_DGRAM --  UDP类型，不保证数据接收的顺序，非可靠连接；
	s = socket(PF_INET, SOCK_DGRAM, 0);
	if (s == SOCKET_ERROR) {
		printf("创建套接字失败\r\n");
		return -2;
	}
	else {
		printf("创建套接字成功\r\n");
	}

	//设置端口号
	int nPort = 8180; // 服务器的端口号
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(nPort);
	//serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	char serverip[50] = { 0 };
	printf("serverip: ");
	scanf("%s", &serverip);
	inet_pton(AF_INET, serverip, (void*)&serverAddr.sin_addr.s_addr);

	// 针对丢包问题，要么减小流量，要么换tcp协议传输，要么做丢包重传的工作

	char filename[MAX_PATH] = { 0 };
	printf("[filename]: ");
	scanf("%s", &filename);
	nRet = fopen_s(&test, filename, "rb");
	if (nRet != 0)
	{
		printf("%s不存在\r\n", filename);
		system("pause");
		return -1;
	}
	else
	{

	}
	nRet = sendto(s, filename, strlen(filename) + 1, 0, (SOCKADDR*)&serverAddr, sizeof(SOCKADDR));
	if (nRet == SOCKET_ERROR || nRet < 0) {
		printf("发送文件名错误\r\n");
		return -1;
	}

	recvfrom(s, (char*)&filenamereceived, sizeof(int), 0, (SOCKADDR*)&temp, &dwRecvSize);
	while (filenamereceived != 1);
	//设置socket的缓冲区
	int nSendBuf = 1024 * 64;
	int nRecvBuf = 1024 * 64;
	setsockopt(s, SOL_SOCKET, SO_SNDBUF, (const char*)&nSendBuf, sizeof(int));
	setsockopt(s, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(int));

	lsend(s, &serverAddr, filename);
	closesocket(s);
	WSACleanup();
	system("pause");
	return 0;
}

int lsend(SOCKET s, SOCKADDR_IN* server_addr, char* filename)
{
	struct Packet Send = { 0 };
	struct Feedback Recv = { 0 };
	int packet_count;
	bool rwnd_zero_flag;
	bool retransmit_flag;
	bool congestion_flag;
	int current_packet;
	int count;
	int nRet;
	SOCKADDR_IN temp{};
	int dwSendSize= sizeof(SOCKADDR);
	int dwRecvSize = sizeof(SOCKADDR_IN);
	FILE* fp;


	packet_count = 1;
	nRet = fopen_s(&fp,filename, "rb");
	if (nRet != 0)
	{
		printf("%s不存在\r\n",filename);
		return -2;
	}
	rwnd_zero_flag = false;//接收窗口标志初始化
	retransmit_flag = false;//重传标志初始化
	current_packet = 1;//下一个需要传的包的序号seq
	//空列表用于暂时保存数据包
	//struct Packet List[5] = {0};
	congestion_flag = false;//阻塞标志
    //添加BUFFER暂时存储上一个发送过的包的数据，当丢包发生时执行重传操作
	char buffer[FILE_SIZE] = { 0 };
	while (true)
	{
		memset(&Send, 0, sizeof(Send));
		Send.seq = packet_count;
		Send.ack = packet_count;
		if (rwnd_zero_flag == false)
		{
			if (retransmit_flag == false)
			{
				count = fread_s(Send.data, sizeof(Send.data), sizeof(char), sizeof(Send.data), fp);
			}
			//需要重传
			else
			{
				Send.ack -= 1;
				Send.seq -= 1;
				packet_count -= 1;
				printf("需要重传的包序号为 seq = %d\r\n", Send.seq);
				strcpy(Send.data, buffer);
			}
			memset(buffer, 0, sizeof(buffer));
			memcpy(buffer, Send.data,sizeof(Send.data));
			current_packet = Send.seq;
			if (count > 0)
			{
				Send.end = 0;
				nRet = sendto(s, (char*)&Send, count + sizeof(int)* 3, 0, (SOCKADDR*)server_addr, sizeof(SOCKADDR));
				if (nRet == SOCKET_ERROR || nRet < 0) {
					printf("发送数据错误\r\n");
					return -1;
				}
			}
			//文件传输结束，发送结束包
			else
			{
				memset(Send.data, 0, sizeof(Send.data));
				Send.end = 1;
				packet_count += 1;
				nRet = sendto(s, (char*)&Send, 1+sizeof(int)* 3, 0, (SOCKADDR*)server_addr, sizeof(SOCKADDR));
				if (nRet == SOCKET_ERROR || nRet < 0) {
					printf("发送数据错误\r\n");
					return -1;
				}
				//等待ACK包
				memset(&Recv, 0, sizeof(Recv));
				count = recvfrom(s, (char*)&Recv, sizeof(Recv), 0, (SOCKADDR*)&temp, &dwRecvSize);
				printf("接收自服务器的数据: ack=%d\r\n", Recv.ack);
				break;
			}
		}
		//接收窗口满了，发确认rwnd的包
		else
		{
			//不需要重传
			if (retransmit_flag == false)
			{
				Send.seq = 0;
				Send.end = 0;
				memset(Send.data, 0, sizeof(Send.data));
				nRet = sendto(s, (char*)&Send, 1+sizeof(int)* 3, 0, (SOCKADDR*)server_addr, sizeof(SOCKADDR));
				if (nRet == SOCKET_ERROR || nRet < 0) {
					printf("发送数据错误\r\n");
					return -1;
				}
			}
			//需要重传
			else
			{
				Send.ack -= 1;
				Send.seq -= 1;
				packet_count -= 1;
				memcpy(Send.data, buffer,sizeof(buffer));
				nRet = sendto(s, (char*)&Send, strlen(Send.data) + 1 + sizeof(int)* 3, 0, (SOCKADDR*)server_addr, sizeof(SOCKADDR));
				if (nRet == SOCKET_ERROR || nRet < 0) {
					printf("发送数据错误\r\n");
					return -1;
				}
			}
			memset(buffer, 0, sizeof(buffer));
			//暂存下要传输的包，用于重传机制
			memcpy(buffer, Send.data,sizeof(Send.data));
			current_packet = Send.seq;
		}
		packet_count += 1;
		//发送成功，等待ack
		memset(&Recv, 0, sizeof(Recv));
		recvfrom(s, (char*)&Recv, sizeof(Recv), 0, (SOCKADDR*)&temp, &dwRecvSize);

		//判断是否需要重传
		if (Recv.ack != current_packet)
		{
			printf("收到重复的ACK包: ack= %d\r\n", Recv.ack);
			retransmit_flag = true;
		}
		else
		{
			retransmit_flag = false;
		}
			
		//判断rwnd是否已经满了
		if (Recv.rwnd == 0)
		{
			rwnd_zero_flag = true;
		}
		else
		{
			rwnd_zero_flag = false;
		}
		printf("接收自服务器的数据: ack=%d rwnd=%d\r\n",Recv.ack,Recv.rwnd);
	}
	printf("文件发送完成，一共发了%d个包\r\n", packet_count);
	fclose(fp);
	return 0;
}

