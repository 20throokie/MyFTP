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
//struct Packet��С 1024+4*3=1036B
struct Feedback
{
	int ack;
	int rwnd;
};

int lget(SOCKET s, SOCKADDR_IN* client_addr, char* filename);

int main()
{
	WSAData wsd;           //��ʼ����Ϣ
	SOCKET s;

	// MTU��̫������֡�ĳ�����46-1500�ֽ�֮�䣬1500����·�������䵥Ԫ
	// ����UDP������������Ϊ1472�ֽڣ�1500-20-8��
	// �����б�׼UDPֵΪ576�ֽڣ�����ڱ���н����ݳ��ȿ�����548�ֽ�����
	int nPort = 8180;      //���ñ�������Ķ˿ں�
	SOCKADDR_IN siLocal{};    //Զ�̷��ͻ���ַ�ͱ������ջ���ַ
	SOCKADDR_IN clientAddr{};
	int dwSendSize = sizeof(SOCKADDR);
	int dwRecvSize = sizeof(SOCKADDR_IN);
	int nRet;
	int temp = 1;

	//����Winsock
	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
		printf("��ʼ������ʧ��\r\n");
		return 0;
	}
	else {
		printf("�����ɹ�\r\n");
	}

	//����socket
	s = socket(PF_INET, SOCK_DGRAM, 0);
	if (s == SOCKET_ERROR) {
		printf("�����׽���ʧ��\r\n");
		closesocket(s);
		WSACleanup();
		return -2;
	}
	else {
		printf("�����׽��ֳɹ�\r\n");
	}

	char on = 1;
	// ���ö˿ڸ���
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	//int nRecvBuf = 4 * 1024 * 1024;//����Ϊ4M
	//setsockopt(s, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(nRecvBuf));

	siLocal.sin_family = AF_INET;
	siLocal.sin_port = htons(nPort);
	siLocal.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	//�󶨱��ص�ַ��socket
	if (bind(s, (SOCKADDR*)&siLocal, sizeof(siLocal)) == SOCKET_ERROR) {
		printf("���׽���ʧ��\r\n");
		closesocket(s);
		WSACleanup();
		return -3;
	}
	else {
		printf("���׽��ֳɹ�\r\n");
	}

	char filename[MAX_PATH] = { 0 };
	// ��ʼ�����ļ��������ѿͻ�����Ϣ���뵽clientAddr��
	nRet=recvfrom(s, filename, sizeof(filename), 0, (SOCKADDR*)&clientAddr, &dwRecvSize);
	if (nRet < 0)
	{
		printf("�ļ������մ���\r\n");
		closesocket(s);
		WSACleanup();
		return -4;
	}
	sendto(s, (char*)&temp, sizeof(int), 0, (SOCKADDR*)&clientAddr, sizeof(SOCKADDR));

	//���÷�����ģʽ
	int imode = 1;
	int rev = ioctlsocket(s, FIONBIO, (u_long*)&imode);
	if (rev == SOCKET_ERROR)
	{
		printf("���÷�����ģʽʧ��\r\n");
		closesocket(s);
		WSACleanup();
		return -1;
	}
	//����socket�Ļ�����
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

		//���������������֪ͨ�ͻ���Ҫ���ط�
		srand((unsigned)time(NULL));
		random = (rand() % 100) + 1;
		if (random == 2)
		{
			printf("������Ѷ�ʧ��%d����,Ҫ��ͻ����ط�\r\n", Recv.seq);
			//������һ������ack
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
			//Ҫ�����Ҫ���������򽫸ð�ֱ�Ӷ������ȴ���һ����Ű��ĵ���
			if (Recv.ack != packet_count - 1)
			{
				printf("����˽��յ�%d��������Ų���ȷ,Ҫ��������ط�", Recv.seq);
				Send.ack = Recv.ack - 1;
				sendto(s, (char*)&Send, sizeof(Send), 0, (SOCKADDR*)&server_addr, sizeof(SOCKADDR));
				continue;
			}
			List.seq = Recv.seq;
			List.ack = Recv.ack;
			List.end = Recv.end;
			memcpy(List.data, Recv.data,sizeof(Recv.data));
			Send.rwnd -= 1;
			//������ϣ�����ACK������
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
		printf("�������ѽ��յ�%d������rwndΪ%d\r\n", Recv.ack, Send.rwnd);
		temp = Recv.end;
		Send.rwnd += 1;
		memset(&List, 0, sizeof(List));
		if (temp == 1) break;
	}

	printf("�ļ�������ɣ�һ������%d����\r\n", packet_count);
	fclose(fp2);
	return 0;
}


