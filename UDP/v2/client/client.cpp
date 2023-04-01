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
//struct Packet��С 1024+4*3=1036B
struct Feedback
{
	int ack;
	int rwnd;
};

int lsend(SOCKET s, SOCKADDR_IN* server_addr, char* filename);

int main()
{
	WSAData wsd;           //��ʼ����Ϣ
	SOCKET s;         //���͵���Ŀ��SOCKET
	int nRet = 0;
	SOCKADDR_IN serverAddr{};    //������socket��ַ
	SOCKADDR_IN temp{};
	int dwSendSize = sizeof(SOCKADDR);
	int dwRecvSize = sizeof(SOCKADDR_IN);
	int filenamereceived=0;
	FILE* test;

	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
		printf("��ʼ������ʧ��\r\n");
		return 0;
	}
	else {
		printf("�����ɹ�\r\n");
	}

	//����socket

	//AF_INET Э����:������Ҫ��ipv4��ַ��32λ�ģ���˿ںţ�16λ�ģ������
	//SOCK_DGRAM --  UDP���ͣ�����֤���ݽ��յ�˳�򣬷ǿɿ����ӣ�
	s = socket(PF_INET, SOCK_DGRAM, 0);
	if (s == SOCKET_ERROR) {
		printf("�����׽���ʧ��\r\n");
		return -2;
	}
	else {
		printf("�����׽��ֳɹ�\r\n");
	}

	//���ö˿ں�
	int nPort = 8180; // �������Ķ˿ں�
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(nPort);
	//serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	char serverip[50] = { 0 };
	printf("serverip: ");
	scanf("%s", &serverip);
	inet_pton(AF_INET, serverip, (void*)&serverAddr.sin_addr.s_addr);

	// ��Զ������⣬Ҫô��С������Ҫô��tcpЭ�鴫�䣬Ҫô�������ش��Ĺ���

	char filename[MAX_PATH] = { 0 };
	printf("[filename]: ");
	scanf("%s", &filename);
	nRet = fopen_s(&test, filename, "rb");
	if (nRet != 0)
	{
		printf("%s������\r\n", filename);
		system("pause");
		return -1;
	}
	else
	{

	}
	nRet = sendto(s, filename, strlen(filename) + 1, 0, (SOCKADDR*)&serverAddr, sizeof(SOCKADDR));
	if (nRet == SOCKET_ERROR || nRet < 0) {
		printf("�����ļ�������\r\n");
		return -1;
	}

	recvfrom(s, (char*)&filenamereceived, sizeof(int), 0, (SOCKADDR*)&temp, &dwRecvSize);
	while (filenamereceived != 1);
	//����socket�Ļ�����
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
		printf("%s������\r\n",filename);
		return -2;
	}
	rwnd_zero_flag = false;//���մ��ڱ�־��ʼ��
	retransmit_flag = false;//�ش���־��ʼ��
	current_packet = 1;//��һ����Ҫ���İ������seq
	//���б�������ʱ�������ݰ�
	//struct Packet List[5] = {0};
	congestion_flag = false;//������־
    //���BUFFER��ʱ�洢��һ�����͹��İ������ݣ�����������ʱִ���ش�����
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
			//��Ҫ�ش�
			else
			{
				Send.ack -= 1;
				Send.seq -= 1;
				packet_count -= 1;
				printf("��Ҫ�ش��İ����Ϊ seq = %d\r\n", Send.seq);
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
					printf("�������ݴ���\r\n");
					return -1;
				}
			}
			//�ļ�������������ͽ�����
			else
			{
				memset(Send.data, 0, sizeof(Send.data));
				Send.end = 1;
				packet_count += 1;
				nRet = sendto(s, (char*)&Send, 1+sizeof(int)* 3, 0, (SOCKADDR*)server_addr, sizeof(SOCKADDR));
				if (nRet == SOCKET_ERROR || nRet < 0) {
					printf("�������ݴ���\r\n");
					return -1;
				}
				//�ȴ�ACK��
				memset(&Recv, 0, sizeof(Recv));
				count = recvfrom(s, (char*)&Recv, sizeof(Recv), 0, (SOCKADDR*)&temp, &dwRecvSize);
				printf("�����Է�����������: ack=%d\r\n", Recv.ack);
				break;
			}
		}
		//���մ������ˣ���ȷ��rwnd�İ�
		else
		{
			//����Ҫ�ش�
			if (retransmit_flag == false)
			{
				Send.seq = 0;
				Send.end = 0;
				memset(Send.data, 0, sizeof(Send.data));
				nRet = sendto(s, (char*)&Send, 1+sizeof(int)* 3, 0, (SOCKADDR*)server_addr, sizeof(SOCKADDR));
				if (nRet == SOCKET_ERROR || nRet < 0) {
					printf("�������ݴ���\r\n");
					return -1;
				}
			}
			//��Ҫ�ش�
			else
			{
				Send.ack -= 1;
				Send.seq -= 1;
				packet_count -= 1;
				memcpy(Send.data, buffer,sizeof(buffer));
				nRet = sendto(s, (char*)&Send, strlen(Send.data) + 1 + sizeof(int)* 3, 0, (SOCKADDR*)server_addr, sizeof(SOCKADDR));
				if (nRet == SOCKET_ERROR || nRet < 0) {
					printf("�������ݴ���\r\n");
					return -1;
				}
			}
			memset(buffer, 0, sizeof(buffer));
			//�ݴ���Ҫ����İ��������ش�����
			memcpy(buffer, Send.data,sizeof(Send.data));
			current_packet = Send.seq;
		}
		packet_count += 1;
		//���ͳɹ����ȴ�ack
		memset(&Recv, 0, sizeof(Recv));
		recvfrom(s, (char*)&Recv, sizeof(Recv), 0, (SOCKADDR*)&temp, &dwRecvSize);

		//�ж��Ƿ���Ҫ�ش�
		if (Recv.ack != current_packet)
		{
			printf("�յ��ظ���ACK��: ack= %d\r\n", Recv.ack);
			retransmit_flag = true;
		}
		else
		{
			retransmit_flag = false;
		}
			
		//�ж�rwnd�Ƿ��Ѿ�����
		if (Recv.rwnd == 0)
		{
			rwnd_zero_flag = true;
		}
		else
		{
			rwnd_zero_flag = false;
		}
		printf("�����Է�����������: ack=%d rwnd=%d\r\n",Recv.ack,Recv.rwnd);
	}
	printf("�ļ�������ɣ�һ������%d����\r\n", packet_count);
	fclose(fp);
	return 0;
}

