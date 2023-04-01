
// clientDlg.h : 头文件
//

#pragma once
#include "afxwin.h"

#define MyMessage WM_USER+100

// CclientDlg 对话框
class CclientDlg : public CDialogEx
{
// 构造
public:
	CclientDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_CLIENT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CString m_Download;
	CString m_address;
//	CString m_Message;
//	CString m_Log;
	CString m_Chat;
	CString m_Download2;
	afx_msg void OnBnClickedenter();
	afx_msg void OnBnClickedsend();
	afx_msg void OnBnClickedsendfile();
	afx_msg void OnBnClickedDirectory();
	afx_msg void OnBnClickedInfo();
	afx_msg void OnBnClickedDownloadfromserver();
protected:
	afx_msg LRESULT OnMyMsg(WPARAM wParam, LPARAM lParam);
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	CListBox m_dirlist;
	afx_msg void OnLbnDblclkList();
//	CEdit m_MesCtrl;
	CListBox m_mesctrl;
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	void SetHScroll();

	CStatic m_1;
	CEdit m_2;
	CButton m_3;
	CStatic m_4;
	CStatic m_5;
	CStatic m_6;
	afx_msg void OnStnClicked3();
};









