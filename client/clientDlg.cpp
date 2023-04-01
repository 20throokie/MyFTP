// clientDlg.cpp : ʵ���ļ�
#include "stdafx.h"
#include "client.h"
#include "clientDlg.h"
#include "afxdialogex.h"
#include "direct.h"
#include <inttypes.h>
#include "stdio.h"
#include "stdlib.h"
#include "winsock2.h"
#include "afxcmn.h"


#define SERVERIP "127.0.0.1"             //�����Ļ��ص�ַ�����������Լ���IP��ַ
#define SERVERPORT 9190
#define PERSEND 4096
#define BUFSIZE 4096
#define PERRECV 4096
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib, "User32.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define MAX_THREAD 3
int SendFile(CListBox& m_mesctrl, SOCKET sock, char filename[32], char path[MAX_PATH],int lastfold, char serpath[MAX_PATH],int line,int record);
//int TcharToChar(const TCHAR * tchar, char * _char);
//int CharToTchar(const char * _char, TCHAR * tchar);
int ListDirectory(CListBox& m_mesctrl, SOCKET sock, char Path[MAX_PATH], int Recursive, int line,int record);
//char* CStringToCharArray(CString cstr);
int RecvFile(CListBox& m_mesctrl, SOCKET sock, char path[MAX_PATH],int line,int record,const char filename[32]);
struct disp_in
{
	bool flag;
	int line;
	char buffer[128];
};
struct disp_in disp[2*MAX_THREAD] = { 0 };
int recvcheck = 0;
//char progress[10] = { 0 ,0 ,0 };
CWnd* cwind;
int taskCnt = 0;
HANDLE hMutex;
HANDLE rw;
HANDLE mutex_disp;
struct recvarg_in
{
	CListBox& m_mesctrl;
	SOCKET s;
	char pth[MAX_PATH];
	char inputname[32];
	char targetpath[MAX_PATH];
};
struct sendarg_in
{
	CListBox& m_mesctrl;
	SOCKET s;
	char fname[32];
	char pth[MAX_PATH];
	char targetpath[MAX_PATH];
};
struct dirarg_in
{
	CListBox& m_mesctrl;
	CListBox& mdir;
	SOCKET s;
};
UINT ThreadSendFile(LPVOID lpParam);
UINT ThreadDir(LPVOID lpParam);
UINT ThreadRecvFile(LPVOID lpParam);

CFont m_font1,m_font2,m_font3;
CRect m_rect;

// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CclientDlg �Ի���



CclientDlg::CclientDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CclientDlg::IDD, pParent)
	, m_Download(_T(""))
	, m_address(_T(""))
	//, m_Message(_T(""))
	//, m_Log(_T(""))
	, m_Chat(_T(""))
	, m_Download2(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CclientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_DOWNLOAD, m_Download);
	DDV_MaxChars(pDX, m_Download, 32);
	DDX_Text(pDX, server_ip, m_address);
	DDV_MaxChars(pDX, m_address, 32);
	//  DDX_Text(pDX, IDC_MESSAGE, m_Message);
	//  DDV_MaxChars(pDX, m_Message, 4096);
	//  DDX_LBString(pDX, IDC_LOG, m_log);
	//  D//  DV_MaxChars(p//  DX, m_log, 100);
	//  DDX_Text(pDX, IDC_LOG, m_Log);
	//  D//  DV_MaxChars(p//  DX, m_Log, 100);
	//  DDX_Text(pDX, IDC_LOG, m_Log);
	//  DDV_MaxChars(pDX, m_Log, 1024);
	DDX_Text(pDX, IDC_CHAT, m_Chat);
	DDV_MaxChars(pDX, m_Chat, 1024);
	DDX_Text(pDX, IDC_DOWNLOADFILE, m_Download2);
	DDV_MaxChars(pDX, m_Download2, 32);
	DDX_Control(pDX, IDC_LIST, m_dirlist);
	//  DDX_Control(pDX, IDC_MESSAGE, m_MesCtrl);
	DDX_Control(pDX, IDC_MESSAGE, m_mesctrl);
	DDX_Control(pDX, IDC_1, m_1);
	DDX_Control(pDX, server_ip, m_2);
	DDX_Control(pDX, enter, m_3);
	DDX_Control(pDX, IDC_2, m_4);
	DDX_Control(pDX, IDC_3, m_5);
	DDX_Control(pDX, IDC_4, m_6);
}

BEGIN_MESSAGE_MAP(CclientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(MyMessage, &CclientDlg::OnMyMsg)
	ON_BN_CLICKED(enter, &CclientDlg::OnBnClickedenter)
	ON_BN_CLICKED(sendfile, &CclientDlg::OnBnClickedsendfile)
	ON_BN_CLICKED(IDC_DIRECTORY, &CclientDlg::OnBnClickedDirectory)
	ON_BN_CLICKED(IDC_INFO, &CclientDlg::OnBnClickedInfo)
	ON_BN_CLICKED(IDC_DOWNLOADFROMSERVER, &CclientDlg::OnBnClickedDownloadfromserver)
	ON_WM_TIMER()
	ON_LBN_DBLCLK(IDC_LIST, &CclientDlg::OnLbnDblclkList)
	ON_WM_SIZE()
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


// CclientDlg ��Ϣ�������

BOOL CclientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	ShowWindow(SW_MINIMIZE);

	// TODO:  �ڴ���Ӷ���ĳ�ʼ������

	//ShowWindow(SW_MAXIMIZE);
	hMutex = CreateMutex(NULL, false, NULL);
	rw = CreateMutex(NULL, false, NULL);
	mutex_disp = CreateMutex(NULL, false, NULL);
	//SetTimer(1, 2000, NULL);
	m_font1.CreatePointFont(100, _T("΢���ź�"));
	m_dirlist.SetFont(&m_font1);
	m_mesctrl.SetFont(&m_font1);
	m_font2.CreatePointFont(108, _T("΢���ź�"));
	m_1.SetFont(&m_font2);
	m_2.SetFont(&m_font2);
	m_3.SetFont(&m_font2);
	m_font3.CreatePointFont(92, _T("΢���ź�"));
	m_4.SetFont(&m_font3);
	m_5.SetFont(&m_font3);
	m_6.SetFont(&m_font3);

	GetClientRect(&m_rect);

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CclientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CclientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();

		//CPaintDC   dc(this);
		//CRect rect;
		//GetClientRect(&rect);
		//CDC   dcMem;
		//dcMem.CreateCompatibleDC(&dc);
		//CBitmap   bmpBackground;
		//bmpBackground.LoadBitmap(IDB_BITMAP1);  //�Ի���ı���ͼƬ  

		//BITMAP   bitmap;
		//bmpBackground.GetBitmap(&bitmap);
		//CBitmap   *pbmpOld = dcMem.SelectObject(&bmpBackground);
		//dc.StretchBlt(0, 0, rect.Width(), rect.Height(), &dcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CclientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CclientDlg::OnBnClickedenter()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	PROCESS_INFORMATION pi = { 0 };//������Ϣ�ṹ
	STARTUPINFO si = { 0 };//������Ϣ�ṹ
	si.cb = sizeof(si);
	si.wShowWindow = SW_SHOW;//��ʾ����
	si.dwFlags = STARTF_USESHOWWINDOW;//������־,��ʾ����
	::CreateProcess(
		TEXT("D:\\MyFTP\\Debug\\server.exe"),//��ִ���ļ�(exe)��ģ��·��
		nullptr,//�����в��� 
		nullptr,//Ĭ�Ͻ��̰�ȫ�� 
		nullptr,//Ĭ���̰߳�ȫ��
		FALSE,  //ָ����ǰ�����ھֲ������Ա��ӽ��̼̳� 
		CREATE_DEFAULT_ERROR_MODE,      //�������̵ı�־,������Ϊ0,����Ҫ
		nullptr,//ʹ�ñ����̵Ļ�������
		TEXT("D:\\MyFTP\\server"),//ʹ�ñ����̵���������Ŀ¼
		&si, &pi);
}



void CclientDlg::OnBnClickedsendfile()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	UpdateData(true);
	if (strlen(m_Download) == 0 || (!strcmp(m_Download, ".")) || (!strcmp(m_Download, "..")))
	{
		WaitForSingleObject(mutex_disp, INFINITE);
		m_mesctrl.InsertString(m_mesctrl.GetCount(), "�����ʽ����");
		ReleaseMutex(mutex_disp);
		return;
	}
	int ret, len;
	WSADATA data;
	SOCKET sockClient;
	struct sockaddr_in addrServer;

	ret = WSAStartup(MAKEWORD(2, 2), &data);
	if (SOCKET_ERROR == ret)
	{
		return;
	}

	sockClient = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == sockClient)
	{
		WSACleanup();
		return;
	}

	//����I/O��������С
	int Send = 1024 * 32, Recv = 1024 * 32;
	ret = setsockopt(sockClient, SOL_SOCKET, SO_SNDBUF, (char*)&Send, sizeof(Send));
	ret = setsockopt(sockClient, SOL_SOCKET, SO_RCVBUF, (char*)&Recv, sizeof(Recv));

	CStringA strA(m_address);
	memset(&addrServer, 0, sizeof(struct sockaddr_in));
	addrServer.sin_family = AF_INET;
	addrServer.sin_addr.S_un.S_addr = inet_addr((char*)strA.GetString());
	addrServer.sin_port = htons(SERVERPORT);

	CString temp;
	temp = _T("");
	m_dirlist.GetText(1, temp);
	if (strlen(temp) <= 2)
	{
		closesocket(sockClient);
		WSACleanup();
		return;
	}
	len = sizeof(struct sockaddr);
	ret = connect(sockClient, (struct sockaddr*)&addrServer, len);
	if (SOCKET_ERROR == ret)
	{
		closesocket(sockClient);
		WSACleanup();
		WaitForSingleObject(mutex_disp, INFINITE);
		m_mesctrl.InsertString(m_mesctrl.GetCount(),"����" + (CStringA)m_address + "ʧ��");
		ReleaseMutex(mutex_disp);
		UpdateData(false);
		UpdateWindow();
		return;
	}
	WaitForSingleObject(mutex_disp, INFINITE);
	m_mesctrl.InsertString(m_mesctrl.GetCount(),"����" + (CStringA)m_address + "�ɹ�...");
	ReleaseMutex(mutex_disp);
	UpdateData(false);
	UpdateWindow();

	WaitForSingleObject(hMutex, INFINITE);
	if (taskCnt >= MAX_THREAD)
	{
		WaitForSingleObject(mutex_disp, INFINITE);
		m_mesctrl.InsertString(m_mesctrl.GetCount(), "�������");
		ReleaseMutex(mutex_disp);
		ReleaseMutex(hMutex);
		return;
	}
	taskCnt++;
	ReleaseMutex(hMutex);

	char sendBuf[BUFSIZE], recvBuf[BUFSIZE];
	memset(sendBuf, 0, sizeof(sendBuf));
	memset(recvBuf, 0, sizeof(recvBuf));
	sendBuf[0] = 2;
	send(sockClient, sendBuf, 1, 0);
	//	CStringA strB(m_Download);
	TCHAR Buffer[MAX_PATH];
	DWORD dwRet;
	//char temp[MAX_PATH] = { 0 };
	dwRet = GetCurrentDirectory(MAX_PATH, Buffer);
	//char* temp2 = CStringToCharArray(m_Download);

	WaitForSingleObject(mutex_disp, INFINITE);
	sendarg_in arg = {m_mesctrl, sockClient,0,0,0 };
	ReleaseMutex(mutex_disp);
	strcpy(arg.fname, m_Download);
	strcpy(arg.pth, Buffer);
	strncpy(arg.targetpath, temp, strlen(temp) - 2);
	strncat(arg.targetpath, "\0", 1);
	SetTimer(1, 2000, NULL);

	AfxBeginThread(ThreadSendFile, (LPVOID)&arg);
	//ret = SendFile(m_Message, sockClient, inputname, (char*)Buffer, 1);

	return;
}


int SendFile(CListBox& m_mesctrl, SOCKET sock, char filename[32], char path[MAX_PATH], int lastfold, char serpath[MAX_PATH],int line,int record)
{
	int ret,temp3;
	char sendBuf[BUFSIZE], recvBuf[BUFSIZE];
	FILE *fp;
	int i, lastsend;
	__int64 totlen,cnt,CurSend;

	if (lastfold == 1)
	{
		memset(sendBuf, 0, sizeof(sendBuf));
		strcpy(sendBuf, serpath);
		send(sock, sendBuf, sizeof(sendBuf), 0);
		WaitForSingleObject(mutex_disp, INFINITE);
		//line=m_mesctrl.GetCount();
		//m_mesctrl.InsertString(line, "...");
		for (int i = 0; i < 2*MAX_THREAD; i++)
		{
			if (disp[i].flag == 0)
			{
				disp[i].flag = 1;
				record = i;
				break;
			}
		}
		ReleaseMutex(mutex_disp);
	}

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
			send(sock, sendBuf, sizeof(sendBuf), 0);
			Sleep(10);
			memset(recvBuf, 0, sizeof(recvBuf));
			recv(sock, recvBuf, 1, 0);
			if (recvBuf[0] != 1)
			{
				WaitForSingleObject(mutex_disp, INFINITE);
				m_mesctrl.InsertString(m_mesctrl.GetCount(),"��������������");
				ReleaseMutex(mutex_disp);
				return -7;
			}
			temp3=ListDirectory(m_mesctrl,sock,temp4, 1, line, record);
			if (temp3 != 0)
			{
				WaitForSingleObject(mutex_disp, INFINITE);
				m_mesctrl.InsertString(m_mesctrl.GetCount(),"�����ļ�ʱ����");
				ReleaseMutex(mutex_disp);
				return -8;
			}
			if (lastfold == 1)
			{
				memset(sendBuf, 0, sizeof(sendBuf));
				sendBuf[0] = 4;
				send(sock, sendBuf, 1, 0);
				WaitForSingleObject(mutex_disp, INFINITE);
				disp[record].flag = 0;
				//m_mesctrl.DeleteString(line);
				ReleaseMutex(mutex_disp);
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
			WaitForSingleObject(mutex_disp, INFINITE);
			m_mesctrl.InsertString(m_mesctrl.GetCount(),"�ļ��򿪴������������ļ���");
			ReleaseMutex(mutex_disp);
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
		WaitForSingleObject(mutex_disp, INFINITE);
		m_mesctrl.InsertString(m_mesctrl.GetCount(),"�����ļ�������");
		ReleaseMutex(mutex_disp);
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
		WaitForSingleObject(mutex_disp, INFINITE);
		m_mesctrl.InsertString(m_mesctrl.GetCount(),"�����ļ��ܳ��ȴ���");
		ReleaseMutex(mutex_disp);
		return -3;
	}

	//memset(recvBuf, 0, sizeof(recvBuf));
	//ret = recv(sock, recvBuf, sizeof(recvBuf), 0);
	//if (SOCKET_ERROR == ret || (strcmp(sendBuf, recvBuf) != 0))
	//{
	//	return -5;
	//}

	//	printf("������������ļ����ȳɹ�\n");

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
		WaitForSingleObject(mutex_disp, INFINITE);
		m_mesctrl.InsertString(m_mesctrl.GetCount(),"�������һ�����ݳ��ȴ���");
		ReleaseMutex(mutex_disp);
		return -4;
	}
	//Sleep(10);

	fp = fopen(temp4, "rb");
	if (NULL == fp)
	{
		return -1;
	}

	char strbuffer[128];
	char progress[8] = { 0 };

	CurSend = 0;
	//ǰcnt-1ÿ�δ�PERSEND�ֽ�
	for (i = 1; i <= cnt - 1; i++)
	{
		memset(sendBuf, 0, sizeof(sendBuf));
		ret = fread(sendBuf, PERSEND, 1, fp);
		if (ret != 1){ fclose(fp); WaitForSingleObject(mutex_disp, INFINITE); m_mesctrl.InsertString(m_mesctrl.GetCount(), "�ļ���ȡʱ����"); ReleaseMutex(mutex_disp); return -5; }

		ret = send(sock, sendBuf, PERSEND, 0);
		if (ret != PERSEND){ fclose(fp); WaitForSingleObject(mutex_disp, INFINITE); m_mesctrl.InsertString(m_mesctrl.GetCount(), "�����ļ����ݴ���"); ReleaseMutex(mutex_disp); return -5; }
		CurSend = CurSend + PERSEND;
		sprintf(progress, "%.2f", 100*(double)CurSend/(double)totlen);
		memset(strbuffer, 0, sizeof(strbuffer));
		strcat(strbuffer, filename);
		strcat(strbuffer, "������...");
		strcat(strbuffer, progress);
		strcat(strbuffer, "%");
		WaitForSingleObject(mutex_disp, INFINITE);
		disp[record].flag = 1;
		disp[record].line = line;
		strcpy(disp[record].buffer, strbuffer);
		ReleaseMutex(mutex_disp);

		//SendMessageTimeout(AfxGetMainWnd()->m_hWnd, MyMessage,false,NULL,SMTO_NORMAL,100,NULL);
		//PostMessage(AfxGetMainWnd()->m_hWnd, MyMessage, false, NULL);
		//MSG msg;
		//if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		//{
		//	if (msg.message == WM_QUIT)
		//		exit(0);
		//	TranslateMessage(&msg);
		//	DispatchMessage(&msg);
		//}
		//else
		//{
		//	// ���ĳЩ�����������г�ʽ
		//}
	}
	

	//���һ�δ� lastsend�ֽ�
	memset(sendBuf, 0, sizeof(sendBuf));
	ret = fread(sendBuf, lastsend, 1, fp);
	if (ret != 1){ fclose(fp); WaitForSingleObject(mutex_disp, INFINITE);  m_mesctrl.InsertString(m_mesctrl.GetCount(), "�ļ���ȡʱ����"); ReleaseMutex(mutex_disp);  return -5; }
	ret = send(sock, sendBuf, lastsend, 0);
	if (ret != lastsend){ fclose(fp); WaitForSingleObject(mutex_disp, INFINITE); m_mesctrl.InsertString(m_mesctrl.GetCount(), "�����ļ����ݴ���"); ReleaseMutex(mutex_disp);  return -5; }

	memset(recvBuf, 0, sizeof(recvBuf));
	recv(sock, recvBuf, 1, 0);
	if (recvBuf[0] != 2)
	{
		WaitForSingleObject(mutex_disp, INFINITE);
		m_mesctrl.InsertString(m_mesctrl.GetCount(),"��������������");
		ReleaseMutex(mutex_disp);
		return -9;
	}
	fclose(fp);
	//Sleep(3000);
	return 0;
}

void CclientDlg::OnBnClickedDirectory()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	char sendBuf[BUFSIZE]; char recvBuf[BUFSIZE];
	UpdateData(true);
	//m_Log = TEXT("");
	int ret, len;
	WSADATA data;
	SOCKET sockClient;
	struct sockaddr_in addrServer;

	ret = WSAStartup(MAKEWORD(2, 2), &data);
	if (SOCKET_ERROR == ret)
	{
		return;
	}

	sockClient = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == sockClient)
	{
		WSACleanup();
		return;
	}

	CStringA strA(m_address);
	memset(&addrServer, 0, sizeof(struct sockaddr_in));
	addrServer.sin_family = AF_INET;
	addrServer.sin_addr.S_un.S_addr = inet_addr((char*)strA.GetString());
	addrServer.sin_port = htons(SERVERPORT);

	len = sizeof(struct sockaddr);
	ret = connect(sockClient, (struct sockaddr*)&addrServer, len);
	if (SOCKET_ERROR == ret)
	{
		closesocket(sockClient);
		WSACleanup();
		WaitForSingleObject(mutex_disp, INFINITE);
		m_mesctrl.InsertString(m_mesctrl.GetCount(),"����" + (CStringA)m_address+ "ʧ��");
		ReleaseMutex(mutex_disp);
		UpdateData(false);
		UpdateWindow();
		return;
	}

	WaitForSingleObject(mutex_disp, INFINITE);
	m_mesctrl.InsertString(m_mesctrl.GetCount(),"����" + (CStringA)m_address + "�ɹ�...");
	ReleaseMutex(mutex_disp);
	UpdateData(false);
	UpdateWindow();

	WaitForSingleObject(hMutex, INFINITE);
	if (taskCnt >= MAX_THREAD)
	{
		WaitForSingleObject(mutex_disp, INFINITE);
		m_mesctrl.InsertString(m_mesctrl.GetCount(), "�������");
		ReleaseMutex(mutex_disp);
		ReleaseMutex(hMutex);
		return;
	}
	taskCnt++;
	ReleaseMutex(hMutex);

	memset(sendBuf, 0, sizeof(sendBuf));
	sendBuf[0] = 1;
	send(sockClient, sendBuf, 1, 0);
	memset(sendBuf, 0, sizeof(sendBuf));
	sendBuf[0] = 0;
	send(sockClient, sendBuf, 1, 0);

	dirarg_in arg = {m_mesctrl, m_dirlist , sockClient };
	AfxBeginThread(ThreadDir, (LPVOID)&arg);

	return;
}

//��TCHARתΪchar  
//*tchar��TCHAR����ָ�룬*_char��char����ָ��  
//int TcharToChar(const TCHAR * tchar, char * _char)
//{
//	int iLength;
//	//��ȡ�ֽڳ���  
//	iLength = WideCharToMultiByte(CP_ACP, 0, tchar, -1, NULL, 0, NULL, NULL);
//	//��tcharֵ����_char   
//	WideCharToMultiByte(CP_ACP, 0, tchar, -1, _char, iLength, NULL, NULL);
//	return 0;
//}


//int CharToTchar(const char * _char, TCHAR * tchar)
//{
//	int iLength;
//
//	iLength = MultiByteToWideChar(CP_ACP, 0, _char, strlen(_char) + 1, NULL, 0);
//	MultiByteToWideChar(CP_ACP, 0, _char, strlen(_char) + 1, tchar, iLength);
//	return 0;
//}

// Recursive��1��ʾ�ݹ���ң������ֻ�г�����Ŀ¼
int ListDirectory(CListBox& m_mesctrl, SOCKET sock, char Path[MAX_PATH], int Recursive,int line,int record)
{
	HANDLE hFind;
	WIN32_FIND_DATA FindFileData;
	char fname[MAX_PATH];
	int Ret = 0;
	int i=0;

	//TCHAR temp[MAX_PATH];
	for (i = 0; i < MAX_PATH; i++)
	{
		fname[i] = *(Path + i);
	}
	//strcpy(fname, Path);
	strcat(fname, "\\");
	strcat(fname, "*.*");
	//CharToTchar(fname, temp);

	// ���ҵ�һ���ļ�
	hFind = FindFirstFile(fname, &FindFileData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		//printf("Error when list %s\n", Path);
		return -1;
	}
	do
	{
		// �����ļ���
		//wcstombs(temp2, FindFileData.cFileName, 260);
		strcpy(fname, Path);
		strcat(fname, "\\");
		strcat(fname, (const char*)FindFileData.cFileName);
		printf("%s\n", fname);
		if (strcmp((const char*)FindFileData.cFileName, ".") != 0 
			&& strcmp((const char*)FindFileData.cFileName, "..") != 0)
		{
			Ret=SendFile(m_mesctrl, sock, (char*)FindFileData.cFileName, Path, 0, NULL, line, record);
			if (Ret != 0) return -2;
		}
		
		//// ����ǵݹ���ң������ļ�������.��..�������ļ���һ��Ŀ¼����ôִ�еݹ����
		//if (Recursive != 0
		//	&& strcmp(temp2, ".") != 0
		//	&& strcmp(temp2, "..") != 0
		//	&& FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		//{
		//	ListDirectory(sock,fname, Recursive);
		//}
		// ������һ���ļ�
		if (FindNextFile(hFind, &FindFileData) == FALSE)
		{
			// ERROR_NO_MORE_FILES ��ʾ�Ѿ�ȫ���������
			if (GetLastError() != ERROR_NO_MORE_FILES)
			{
				return -1;
			}
			else
			{
				Ret = 0;
			}
			break;
		}
	} while (TRUE);

	// �رվ��
	FindClose(hFind);
	return Ret;
}


//char* CStringToCharArray(CString cstr)
//{
//	int len = WideCharToMultiByte(CP_ACP, 0, cstr, cstr.GetLength(), NULL, 0, NULL, NULL);
//	char* chars = new char[len];
//	WideCharToMultiByte(CP_ACP, 0, cstr, cstr.GetLength(), chars, len, NULL, NULL);
//	chars[len] = '\0';
//	return chars;
//}


void CclientDlg::OnBnClickedInfo()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	UpdateData(true);
	int ret, len,bytes;
	WSADATA data;
	SOCKET sockClient;
	struct sockaddr_in addrServer;


	ret = WSAStartup(MAKEWORD(2, 2), &data);
	if (SOCKET_ERROR == ret)
	{
		return;
	}

	sockClient = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == sockClient)
	{
		WSACleanup();
		return;
	}

	CStringA strA(m_address);
	memset(&addrServer, 0, sizeof(struct sockaddr_in));
	addrServer.sin_family = AF_INET;
	addrServer.sin_addr.S_un.S_addr = inet_addr((char*)strA.GetString());
	addrServer.sin_port = htons(SERVERPORT);

	len = sizeof(struct sockaddr);
	ret = connect(sockClient, (struct sockaddr*)&addrServer, len);
	if (SOCKET_ERROR == ret)
	{
		closesocket(sockClient);
		WSACleanup();
		WaitForSingleObject(mutex_disp, INFINITE);
		m_mesctrl.InsertString(m_mesctrl.GetCount(),"����" + (CStringA)m_address + "ʧ��");
		ReleaseMutex(mutex_disp);
		UpdateData(false);
		return;
	}

	WaitForSingleObject(mutex_disp, INFINITE);
	m_mesctrl.InsertString(m_mesctrl.GetCount(),"����" + (CStringA)m_address + "�ɹ�...");
	ReleaseMutex(mutex_disp);
	UpdateData(false);
	UpdateWindow();

	char sendBuf[BUFSIZE], recvBuf[BUFSIZE];
	memset(sendBuf, 0, sizeof(sendBuf));
	memset(recvBuf, 0, sizeof(recvBuf));
	sendBuf[0] = 3;
	send(sockClient, sendBuf, 1, 0);
	memset(sendBuf, 0, sizeof(sendBuf));
	strncpy_s(sendBuf, m_Chat, sizeof(sendBuf));
	bytes=send(sockClient, sendBuf, sizeof(sendBuf), 0);
	WaitForSingleObject(mutex_disp, INFINITE);
	if (bytes == sizeof(sendBuf))
		m_mesctrl.InsertString(m_mesctrl.GetCount(),"�������������Ϣ�ɹ�");
	else
		m_mesctrl.InsertString(m_mesctrl.GetCount(),"�������������Ϣʧ��");
	ReleaseMutex(mutex_disp);
	UpdateData(false);
	closesocket(sockClient);
	WSACleanup();
	return;
}

int RecvFile(CListBox& m_mesctrl, SOCKET sock, char path[MAX_PATH],int line,int record, const char filename[32])
{
	if (recvcheck == 0)
	{
		WaitForSingleObject(mutex_disp, INFINITE);
		//line = m_mesctrl.GetCount();
		//m_mesctrl.InsertString(line, "...");
		for (int i = 0; i < 2 * MAX_THREAD; i++)
		{
			if (disp[i].flag == 0)
			{
				disp[i].flag = 1;
				record = i;
				break;
			}
		}
		ReleaseMutex(mutex_disp);
		recvcheck++;
	}
	int ret, curRecv, lastsend;
	FILE *fp;
	char sendBuf[BUFSIZE], recvBuf[BUFSIZE];
	__int64 totlen, totRecv;

	char temp[MAX_PATH];
	strcpy(temp, path);
	strcat(temp, "\\");

	memset(recvBuf, 0, sizeof(recvBuf));
	recv(sock, recvBuf, 1, 0);
	if (recvBuf[0] == 3)
	{
		memset(recvBuf, 0, sizeof(recvBuf));
		recv(sock, recvBuf, sizeof(recvBuf), 0);
		strncat(temp, recvBuf, strlen(recvBuf) + 1);
		memset(sendBuf, 0, sizeof(sendBuf));
		sendBuf[0] = 1;
		send(sock, sendBuf, 1, 0);
		_mkdir(temp);
		while (1)
		{
			ret = RecvFile(m_mesctrl, sock, temp, line, record,filename);
			if (ret == -6)
			{
				break;
			}
			else if (ret == -7)
			{
				//printf("�ر��ļ���%s\r\n", recvBuf);
				return 0;
			}
			else if (ret!=0)
			{
				return -10;
			}
		}
		WaitForSingleObject(mutex_disp, INFINITE);
		disp[record].flag = 0;
		//m_mesctrl.DeleteString(line);
		ReleaseMutex(mutex_disp);
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
		WaitForSingleObject(mutex_disp, INFINITE);
		m_mesctrl.InsertString(m_mesctrl.GetCount(),"������ļ�������");
		ReleaseMutex(mutex_disp);
		return -8;
	}
	else if (recvBuf[0] != 2)
	{
		WaitForSingleObject(mutex_disp, INFINITE);
		m_mesctrl.InsertString(m_mesctrl.GetCount(),"��������������");
		ReleaseMutex(mutex_disp);
		return -9;
	}

	memset(recvBuf, 0, sizeof(recvBuf));
	ret = recv(sock, recvBuf, sizeof(recvBuf), 0);
	if (SOCKET_ERROR == ret)
	{
		WaitForSingleObject(mutex_disp, INFINITE);
		m_mesctrl.InsertString(m_mesctrl.GetCount(),"�����ļ�������");
		ReleaseMutex(mutex_disp);
		return -1;
	}

	strncat(temp, recvBuf, strlen(recvBuf) + 1);
	fp = fopen(temp, "wb");
	if (NULL == fp)
	{
		WaitForSingleObject(mutex_disp, INFINITE);
		m_mesctrl.InsertString(m_mesctrl.GetCount(),"���ļ�����");
		ReleaseMutex(mutex_disp);
		return -3;
	}

	memset(recvBuf, 0, sizeof(recvBuf));
	ret = recv(sock, recvBuf, sizeof(recvBuf), 0);
	if (SOCKET_ERROR == ret)
	{
		WaitForSingleObject(mutex_disp, INFINITE);
		m_mesctrl.InsertString(m_mesctrl.GetCount(),"�����ļ����ȴ���");
		ReleaseMutex(mutex_disp);
		return -4;
	}
	//printf("�ɹ����տͻ��˷������ļ�����: %s\n", recvBuf);
	totlen = _atoi64(recvBuf);

	memset(recvBuf, 0, sizeof(recvBuf));
	ret = recv(sock, recvBuf, sizeof(recvBuf), 0);
	if (SOCKET_ERROR == ret)
	{
		WaitForSingleObject(mutex_disp, INFINITE);
		m_mesctrl.InsertString(m_mesctrl.GetCount(),"�������һ�����ݳ��ȴ���");
		ReleaseMutex(mutex_disp);
		return -4;
	}
	//printf("���һ��Ҫ���͵��ֽ�%s\r\n", recvBuf);
	lastsend = atoi(recvBuf);

	char progress[8] = { 0 };
	char strbuffer[128];

	totRecv = 0;
	//ѭ������,ֱ���������(totrecv==totlen)
	char curfname[32] = { 0 };
	char ext[32] = { 0 };
	_splitpath(temp, NULL, NULL, curfname, ext);
	while (totRecv + PERRECV <= totlen)
	{
		memset(recvBuf, 0, sizeof(recvBuf));
		curRecv = recv(sock, recvBuf, PERRECV, 0);
		if (SOCKET_ERROR == curRecv){ fclose(fp); WaitForSingleObject(mutex_disp, INFINITE); m_mesctrl.InsertString(m_mesctrl.GetCount(), "�����ļ����ݴ���"); ReleaseMutex(mutex_disp);  return -2; }

		totRecv = totRecv + curRecv;
		ret = fwrite(recvBuf, curRecv, 1, fp);
		if (ret != 1){ fclose(fp); WaitForSingleObject(mutex_disp, INFINITE); m_mesctrl.InsertString(m_mesctrl.GetCount(), "д���ļ����ݴ���"); ReleaseMutex(mutex_disp);  return -2; }
		if (totRecv >= totlen) break;
		sprintf(progress, "%.2f", 100 * (double)totRecv / (double)totlen);
		memset(strbuffer, 0, sizeof(strbuffer));
		strcat(strbuffer, curfname);
		strcat(strbuffer, ext);
		strcat(strbuffer, " ������...");
		strcat(strbuffer, progress);
		strcat(strbuffer, "%");
		WaitForSingleObject(mutex_disp, INFINITE);
		disp[record].flag = 1;
		disp[record].line = line;
		strcpy(disp[record].buffer, strbuffer);
		ReleaseMutex(mutex_disp);

		//MSG msg;
		//if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		//{
		//	if (msg.message == WM_QUIT)
		//		exit(0);
		//	TranslateMessage(&msg);
		//	DispatchMessage(&msg);
		//}
		//else
		//{
		//	// ���ĳЩ�����������г�ʽ 
		//}
	}
	memset(recvBuf, 0, sizeof(recvBuf));
	curRecv = recv(sock, recvBuf, lastsend, 0);
	if (SOCKET_ERROR == curRecv){ fclose(fp); WaitForSingleObject(mutex_disp, INFINITE); m_mesctrl.InsertString(m_mesctrl.GetCount(), "�����ļ����ݴ���"); ReleaseMutex(mutex_disp);  return -2; }
	ret = fwrite(recvBuf, curRecv, 1, fp);
	if (ret != 1){ fclose(fp); WaitForSingleObject(mutex_disp, INFINITE); m_mesctrl.InsertString(m_mesctrl.GetCount(), "д���ļ����ݴ���"); ReleaseMutex(mutex_disp);  return -2; }

	//ȷ��
	memset(sendBuf, 0, sizeof(sendBuf));
	sendBuf[0] = 2;
	send(sock, sendBuf, 1, 0);
	fclose(fp);
	//printf("�ɹ������ļ�\r\n");
	return 0;
}

void CclientDlg::OnBnClickedDownloadfromserver()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	UpdateData(true);
	if (strlen(m_Download2) == 0 || (!strcmp(m_Download2, ".")) || (!strcmp(m_Download2, "..")))
	{
		WaitForSingleObject(mutex_disp, INFINITE);
		m_mesctrl.InsertString(m_mesctrl.GetCount(), "�����ʽ����");
		ReleaseMutex(mutex_disp);
		return;
	}
	int ret, len;
	WSADATA data;
	SOCKET sockClient;
	struct sockaddr_in addrServer;


	ret = WSAStartup(MAKEWORD(2, 2), &data);
	if (SOCKET_ERROR == ret)
	{
		return;
	}

	sockClient = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == sockClient)
	{
		WSACleanup();
		return;
	}
	int Send = 1024 * 32, Recv = 1024 * 32;
	ret = setsockopt(sockClient, SOL_SOCKET, SO_SNDBUF, (char*)&Send, sizeof(Send));
	ret = setsockopt(sockClient, SOL_SOCKET, SO_RCVBUF, (char*)&Recv, sizeof(Recv));

	CStringA strA(m_address);
	memset(&addrServer, 0, sizeof(struct sockaddr_in));
	addrServer.sin_family = AF_INET;
	addrServer.sin_addr.S_un.S_addr = inet_addr((char*)strA.GetString());
	addrServer.sin_port = htons(SERVERPORT);

	CString temp;
	temp = _T("");
	m_dirlist.GetText(1, temp);
	if (strlen(temp) <= 2)
	{
		closesocket(sockClient);
		WSACleanup();
		return;
	}
	len = sizeof(struct sockaddr);
	ret = connect(sockClient, (struct sockaddr*)&addrServer, len);
	if (SOCKET_ERROR == ret)
	{
		closesocket(sockClient);
		WSACleanup();
		WaitForSingleObject(mutex_disp, INFINITE);
		m_mesctrl.InsertString(m_mesctrl.GetCount(),"����" + (CStringA)m_address + "ʧ��");
		ReleaseMutex(mutex_disp);
		UpdateData(false);
		return;
	}

	WaitForSingleObject(mutex_disp, INFINITE);
	m_mesctrl.InsertString(m_mesctrl.GetCount(),"����" + (CStringA)m_address + "�ɹ�...");
	ReleaseMutex(mutex_disp);
	UpdateData(false);
	UpdateWindow();

	WaitForSingleObject(hMutex, INFINITE);
	if (taskCnt >= MAX_THREAD)
	{
		WaitForSingleObject(mutex_disp, INFINITE);
		m_mesctrl.InsertString(m_mesctrl.GetCount(), "�������");
		ReleaseMutex(mutex_disp);
		ReleaseMutex(hMutex);
		return;
	}
	taskCnt++;
	ReleaseMutex(hMutex);

	char sendBuf[BUFSIZE], recvBuf[BUFSIZE];
	memset(sendBuf, 0, sizeof(sendBuf));
	memset(recvBuf, 0, sizeof(recvBuf));
	sendBuf[0] = 4;
	send(sockClient, sendBuf, 1, 0);
	TCHAR Buffer[MAX_PATH];
	DWORD dwRet;
	dwRet = GetCurrentDirectory(MAX_PATH, Buffer);

	WaitForSingleObject(mutex_disp, INFINITE);
	recvarg_in arg = {m_mesctrl, sockClient, 0 , 0 ,0 };
	ReleaseMutex(mutex_disp);
	strcpy(arg.pth, Buffer);
	strncpy_s(arg.inputname, m_Download2, sizeof(arg.inputname));
	strncpy(arg.targetpath, temp, strlen(temp) - 2);
	strncat(arg.targetpath, "\0", 1);
	//SetTimer(2, 2000, NULL);
	SetTimer(1, 2000, NULL);

	AfxBeginThread(ThreadRecvFile, (LPVOID)&arg);
	return;
}

LRESULT CclientDlg::OnMyMsg(WPARAM wParam, LPARAM lParam)
{
	//if (lParam != NULL)
	//{
	//	KillTimer((int)lParam);
	//}
	if ((int)lParam == 0)
	{
		UpdateData(false);
		UpdateWindow();
	}
	if ((int)lParam == 1)
	{
		SetHScroll();
	}
	return 0;
}


void CclientDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ
	//if (nIDEvent == 1)
	//{
	//	m_Message += "������";
	//	m_Message += progress;
	//	m_Message += "%...\r\n";
	//	GetDlgItem(IDC_MESSAGE)->PostMessage(WM_VSCROLL, SB_BOTTOM, 0);
	//	UpdateData(false);
	//	UpdateWindow();
	//}
	//else if (nIDEvent == 2)
	//{
	//	m_Message += "������";
	//	m_Message += progress;
	//	m_Message += "%...\r\n";
	//	GetDlgItem(IDC_MESSAGE)->PostMessage(WM_VSCROLL, SB_BOTTOM, 0);
	//	UpdateData(false);
	//	UpdateWindow();
	//}
	WaitForSingleObject(hMutex,INFINITE);
	if (taskCnt == 0)
	{
		KillTimer(1);
		for (int i = 0; i < 2 * MAX_THREAD; i++)
		{
			disp[i].flag = 0;
		}
	}
	ReleaseMutex(hMutex);
	GetDlgItem(IDC_MESSAGE)->PostMessage(WM_VSCROLL, SB_BOTTOM, 0);
	WaitForSingleObject(mutex_disp, INFINITE);
	for (int i = 0; i < 2*MAX_THREAD; i++)
	{
		if (disp[i].flag == 1)
		{
			if (disp[i].line>=1)
			{
				m_mesctrl.DeleteString(disp[i].line);
				m_mesctrl.InsertString(disp[i].line, disp[i].buffer);
			}
			disp[i].flag = 0;
		}
	}
	ReleaseMutex(mutex_disp);
	UpdateData(false);
	UpdateWindow();
	CDialogEx::OnTimer(nIDEvent);
}


void CclientDlg::OnLbnDblclkList()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	CString strText;
	int nCurSel;
	nCurSel = m_dirlist.GetCurSel();    // ��ȡ��ǰѡ���б���
	m_dirlist.GetText(nCurSel, strText);    // ��ȡѡ���б�����ַ���
	//if (strlen(strText) == 0)
	//{
	//	return;
	//}

	UpdateData(true);
	int ret, len;
	WSADATA data;
	SOCKET sockClient;
	struct sockaddr_in addrServer;

	ret = WSAStartup(MAKEWORD(2, 2), &data);
	if (SOCKET_ERROR == ret)
	{
		return;
	}

	sockClient = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == sockClient)
	{
		WSACleanup();
		return;
	}

	CStringA strA(m_address);
	memset(&addrServer, 0, sizeof(struct sockaddr_in));
	addrServer.sin_family = AF_INET;
	addrServer.sin_addr.S_un.S_addr = inet_addr((char*)strA.GetString());
	addrServer.sin_port = htons(SERVERPORT);

	len = sizeof(struct sockaddr);
	ret = connect(sockClient, (struct sockaddr*)&addrServer, len);
	if (SOCKET_ERROR == ret)
	{
		closesocket(sockClient);
		WSACleanup();
		WaitForSingleObject(mutex_disp, INFINITE);
		m_mesctrl.InsertString(m_mesctrl.GetCount(),"����" + (CStringA)m_address + "ʧ��");
		ReleaseMutex(mutex_disp);
		UpdateData(false);
		return;
	}

	WaitForSingleObject(mutex_disp, INFINITE);
	m_mesctrl.InsertString(m_mesctrl.GetCount(),"����" + (CStringA)m_address + "�ɹ�...");
	ReleaseMutex(mutex_disp);
	UpdateData(false);

	WaitForSingleObject(hMutex, INFINITE);
	if (taskCnt >= MAX_THREAD)
	{
		WaitForSingleObject(mutex_disp, INFINITE);
		m_mesctrl.InsertString(m_mesctrl.GetCount(), "�������");
		ReleaseMutex(mutex_disp);
		ReleaseMutex(hMutex);
		return;
	}
	taskCnt++;
	ReleaseMutex(hMutex);

	char sendBuf[BUFSIZE];
	char recvBuf[BUFSIZE];
	memset(sendBuf, 0, sizeof(sendBuf));
	sendBuf[0] = 1;
	send(sockClient, sendBuf, 1, 0);
	memset(sendBuf, 0, sizeof(sendBuf));
	sendBuf[0] = 1;
	send(sockClient, sendBuf, 1, 0);

	memset(sendBuf, 0, sizeof(sendBuf));
	strcpy(sendBuf, strText);
	send(sockClient, sendBuf, sizeof(sendBuf), 0);
	memset(recvBuf, 0, sizeof(recvBuf));
	//recv(sockClient, recvBuf, 1, 0);
	//if (recvBuf[0] != -2)
	//{
	//	m_dirlist.ResetContent();
	//	while (1)
	//	{
	//		memset(recvBuf, 0, sizeof(recvBuf));
	//		recv(sockClient, recvBuf, sizeof(recvBuf), 0);
	//		if (recvBuf[0] == -1) break;
	//		m_dirlist.AddString(recvBuf);
	//	}
	//	m_Message += "Ŀ¼��ʾ�ɹ�\r\n";
	//}
	//else
	//{
	//	m_Message += "��ѡ����ȷ���ļ���\r\n";
	//}
	//UpdateData(false);
	//closesocket(sockClient);
	//WSACleanup();

	dirarg_in arg = {m_mesctrl, m_dirlist, sockClient };
	AfxBeginThread(ThreadDir, (LPVOID)&arg);
	return;
}

UINT ThreadSendFile(LPVOID lpParam)
{
	sendarg_in sendarg = *((sendarg_in*)lpParam);
	SOCKET sockClient = sendarg.s;
	int temp;
	char strbuffer[32];
	memset(strbuffer, 0, sizeof(strbuffer));
	strcat(strbuffer, "��ʼ���ͣ�");
	strcat(strbuffer, sendarg.fname);
	WaitForSingleObject(mutex_disp, INFINITE);
	sendarg.m_mesctrl.InsertString(sendarg.m_mesctrl.GetCount(),strbuffer);
	temp = sendarg.m_mesctrl.GetCount() - 1;
	sendarg.m_mesctrl.InsertString(sendarg.m_mesctrl.GetCount(), "");
	sendarg.m_mesctrl.InsertString(sendarg.m_mesctrl.GetCount(), "");
	ReleaseMutex(mutex_disp);
	SendMessageTimeout(AfxGetMainWnd()->m_hWnd, MyMessage, false, 0, SMTO_NORMAL, 100, NULL);
	char inputname[MAX_PATH] = { 0 };
	strcpy(inputname, sendarg.fname);
	char Buffer[MAX_PATH] = { 0 };
	strcpy(Buffer, sendarg.pth);
	int ret=-10;
	ret = SendFile(sendarg.m_mesctrl,sockClient, inputname, (char*)Buffer, 1, sendarg.targetpath,temp,NULL);
	//KillTimer(1);
	WaitForSingleObject(mutex_disp, INFINITE);
	if (ret != 0)
	{
		memset(strbuffer, 0, sizeof(strbuffer));
		strcat(strbuffer, inputname);
		strcat(strbuffer, "����ʧ��");
		sendarg.m_mesctrl.DeleteString(temp + 1);
		sendarg.m_mesctrl.InsertString(temp+1,strbuffer);
	}
	else
	{
		memset(strbuffer, 0, sizeof(strbuffer));
		strcat(strbuffer, inputname);
		strcat(strbuffer, "����ɹ�");
		sendarg.m_mesctrl.DeleteString(temp + 1);
		sendarg.m_mesctrl.InsertString(temp+1,strbuffer);
	}
	ReleaseMutex(mutex_disp);
	SendMessageTimeout(AfxGetMainWnd()->m_hWnd, MyMessage, false, 0, SMTO_NORMAL, 100, NULL);
	closesocket(sockClient);
	WSACleanup();
	WaitForSingleObject(hMutex, INFINITE);
	taskCnt--;
	ReleaseMutex(hMutex);
	SendMessageTimeout(AfxGetMainWnd()->m_hWnd, MyMessage, false, 1, SMTO_NORMAL, 100, NULL);
	return 0;
}
UINT ThreadDir(LPVOID lpParam)
{
	dirarg_in dirarg = *((dirarg_in*)lpParam);
	char sendBuf[BUFSIZE];
	char recvBuf[BUFSIZE];
	memset(recvBuf, 0, sizeof(recvBuf));
	recv(dirarg.s, recvBuf, 1, 0);
	if (recvBuf[0] != -2)
	{
		dirarg.mdir.ResetContent();
		while (1)
		{
			memset(recvBuf, 0, sizeof(recvBuf));
			recv(dirarg.s, recvBuf, sizeof(recvBuf), 0);
			if (recvBuf[0] == -1) break;
			dirarg.mdir.AddString(recvBuf);
		}
		WaitForSingleObject(mutex_disp, INFINITE);
		dirarg.m_mesctrl.InsertString(dirarg.m_mesctrl.GetCount(),"Ŀ¼��ʾ�ɹ�");
		dirarg.m_mesctrl.InsertString(dirarg.m_mesctrl.GetCount(), "");
		ReleaseMutex(mutex_disp);
	}
	else
	{
		WaitForSingleObject(mutex_disp, INFINITE);
		dirarg.m_mesctrl.InsertString(dirarg.m_mesctrl.GetCount(),"Ŀ¼��ʾ����");
		dirarg.m_mesctrl.InsertString(dirarg.m_mesctrl.GetCount(), "");
		ReleaseMutex(mutex_disp);
	}
	SendMessageTimeout(AfxGetMainWnd()->m_hWnd, MyMessage, false, 0, SMTO_NORMAL, 100, NULL);
	closesocket(dirarg.s);
	WSACleanup();
	WaitForSingleObject(hMutex, INFINITE);
	taskCnt--;
	ReleaseMutex(hMutex);
	SendMessageTimeout(AfxGetMainWnd()->m_hWnd, MyMessage, false, 1, SMTO_NORMAL, 100, NULL);
	return 0;
}
UINT ThreadRecvFile(LPVOID lpParam)
{
	recvarg_in recvarg = *((recvarg_in*)lpParam);
	char sendBuf[BUFSIZE], recvBuf[BUFSIZE];
	int ret,temp;
	memset(sendBuf, 0, sizeof(sendBuf));
	memset(recvBuf, 0, sizeof(recvBuf));
	char strbuffer[32];
	memset(strbuffer, 0, sizeof(strbuffer));
	strcat(strbuffer, "��ʼ���գ�");
	strcat(strbuffer, recvarg.inputname);
	WaitForSingleObject(mutex_disp, INFINITE);
	recvarg.m_mesctrl.InsertString(recvarg.m_mesctrl.GetCount(),strbuffer);
	temp = recvarg.m_mesctrl.GetCount() - 1;
	recvarg.m_mesctrl.InsertString(recvarg.m_mesctrl.GetCount(), "");
	recvarg.m_mesctrl.InsertString(recvarg.m_mesctrl.GetCount(), "");
	ReleaseMutex(mutex_disp);
	memset(sendBuf, 0, sizeof(sendBuf));
	strncpy_s(sendBuf, recvarg.inputname, sizeof(sendBuf));
	send(recvarg.s, sendBuf, sizeof(sendBuf), 0);
	memset(sendBuf, 0, sizeof(sendBuf));
	strcpy(sendBuf, recvarg.targetpath);
	send(recvarg.s, sendBuf, sizeof(sendBuf), 0);
	WaitForSingleObject(rw, INFINITE);
	recvcheck = 0;
	ret = RecvFile(recvarg.m_mesctrl,recvarg.s, recvarg.pth, temp, NULL, recvarg.inputname);
	recvcheck = 0;
	ReleaseMutex(rw);
	SendMessageTimeout(AfxGetMainWnd()->m_hWnd, MyMessage, false, 0, SMTO_NORMAL, 100, NULL);
	WaitForSingleObject(mutex_disp, INFINITE);
	if (ret == 0)
	{
		memset(strbuffer, 0, sizeof(strbuffer));
		strcat(strbuffer, recvarg.inputname);
		strcat(strbuffer, "���ճɹ�");
		recvarg.m_mesctrl.DeleteString(temp + 1);
		recvarg.m_mesctrl.InsertString(temp+1,strbuffer);
	}
	else
	{
		memset(strbuffer, 0, sizeof(strbuffer));
		strcat(strbuffer, recvarg.inputname);
		strcat(strbuffer, "����ʧ��");
		recvarg.m_mesctrl.DeleteString(temp + 1);
		recvarg.m_mesctrl.InsertString(temp+1,strbuffer);
	}
	ReleaseMutex(mutex_disp);
	SendMessageTimeout(AfxGetMainWnd()->m_hWnd, MyMessage, false, 0, SMTO_NORMAL, 100, NULL);
	closesocket(recvarg.s);
	WSACleanup();
	WaitForSingleObject(hMutex, INFINITE);
	taskCnt--;
	ReleaseMutex(hMutex);
	SendMessageTimeout(AfxGetMainWnd()->m_hWnd, MyMessage, false, 1, SMTO_NORMAL, 100, NULL);
	return 0;
}



void CclientDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	// TODO:  �ڴ˴������Ϣ����������

	//CWnd* pWnd = GetDlgItem(IDC_MESSAGE);
	//if (GetDlgItem(IDC_MESSAGE)->GetSafeHwnd())
	//{
	//	GetDlgItem(IDC_MESSAGE)->MoveWindow(53, 720, 1810, 250);
	//}
	//if (GetDlgItem(IDC_3)->GetSafeHwnd())
	//{
	//	GetDlgItem(IDC_3)->MoveWindow(55, 690, 90,30);
	//}
	//if (GetDlgItem(IDC_LIST)->GetSafeHwnd())
	//{
	//	GetDlgItem(IDC_LIST)->MoveWindow(53, 200, 900, 400);
	//}
	//if (GetDlgItem(IDC_DIRECTORY)->GetSafeHwnd())
	//{
	//	GetDlgItem(IDC_DIRECTORY)->MoveWindow(840, 615, 114, 36);
	//}
	//if (GetDlgItem(IDC_2)->GetSafeHwnd())
	//{
	//	GetDlgItem(IDC_2)->MoveWindow(55, 170, 150, 30);
	//}
	//if (GetDlgItem(IDC_1)->GetSafeHwnd())
	//{
	//	GetDlgItem(IDC_1)->MoveWindow(55, 50, 200, 100);
	//}
	//if (GetDlgItem(server_ip)->GetSafeHwnd())
	//{
	//	GetDlgItem(server_ip)->MoveWindow(200, 50, 200, 35);
	//}
	//if (GetDlgItem(enter)->GetSafeHwnd())
	//{
	//	GetDlgItem(enter)->MoveWindow(1540, 46, 320, 40);
	//}
	//if (GetDlgItem(IDC_DOWNLOAD)->GetSafeHwnd())
	//{
	//	GetDlgItem(IDC_DOWNLOAD)->MoveWindow(1120, 200, 300, 100);
	//}
	//if (GetDlgItem(sendfile)->GetSafeHwnd())
	//{
	//	GetDlgItem(sendfile)->MoveWindow(1300, 310, 114, 36);
	//}
	//if (GetDlgItem(IDC_DOWNLOADFILE)->GetSafeHwnd())
	//{
	//	GetDlgItem(IDC_DOWNLOADFILE)->MoveWindow(1550, 200, 300, 100);
	//}
	//if (GetDlgItem(IDC_DOWNLOADFROMSERVER)->GetSafeHwnd())
	//{
	//	GetDlgItem(IDC_DOWNLOADFROMSERVER)->MoveWindow(1730, 310, 114, 36);
	//}
	//if (GetDlgItem(IDC_CHAT)->GetSafeHwnd())
	//{
	//	GetDlgItem(IDC_CHAT)->MoveWindow(1120, 420, 730, 180);
	//}
	//if (GetDlgItem(IDC_INFO)->GetSafeHwnd())
	//{
	//	GetDlgItem(IDC_INFO)->MoveWindow(1740, 615, 114, 36);
	//}
	//if (GetDlgItem(IDC_4)->GetSafeHwnd())
	//{
	//	GetDlgItem(IDC_4)->MoveWindow(1120, 170, 200, 100);
	//}
}



HBRUSH CclientDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  �ڴ˸��� DC ���κ�����
	if (pWnd->GetDlgCtrlID() == IDC_1)
	{
		//pDC->SetTextColor(RGB(0,0,0));//����������ɫ
		//pDC->SetBkColor(RGB(234,234,234));//���ñ�����ɫ
		//pDC->SetBkMode(TRANSPARENT);//���ñ���͸��
	}
	if (nCtlColor == CTLCOLOR_STATIC)
	{
		pDC->SetBkMode(TRANSPARENT);
		return (HBRUSH)::GetStockObject(NULL_BRUSH);
	}
	// TODO:  ���Ĭ�ϵĲ������軭�ʣ��򷵻���һ������
	return hbr;
}

void CclientDlg::SetHScroll()
{
	CDC* dc=GetDC();
	SIZE s;
	int index;
	CString str;
	long temp;
	for (index = 0; index< m_dirlist.GetCount(); index++)
	{
		m_dirlist.GetText(index, str);
		s = dc->GetTextExtent(str, str.GetLength() + 1);   // ��ȡ�ַ��������ش�С
		// ����µ��ַ�����ȴ�����ǰ��ˮƽ��������ȣ����������ù��������
		// IDC_LISTBOXΪm_List����ԴID
		temp = (long)SendDlgItemMessage(IDC_LIST, LB_GETHORIZONTALEXTENT, 0, 0); //temp�õ��������Ŀ��
		if (s.cx > temp)
		{
			SendDlgItemMessage(IDC_LIST, LB_SETHORIZONTALEXTENT, (WPARAM)s.cx, 0);
		}
	}
	ReleaseDC(dc);
}

