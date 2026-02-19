
// kpaxkkk01Dlg.h: 헤더 파일
//

// kpaxkkk01Dlg.h 상단에 추가
#include <string>
#define WM_UPDATE_PROGRESS (WM_USER + 100)
#define WM_DOWNLOAD_COMPLETE (WM_USER + 101)

struct DownloadRequest {
	HWND hMainWnd;
	std::wstring source;
	std::wstring target;
	int nItemIndex;
	int nLastPercent; 
	DWORD dwLastTick; // 마지막으로 데이터가 움직인 시간 저장
};

#pragma once


// Ckpaxkkk01Dlg 대화 상자
class Ckpaxkkk01Dlg : public CDialogEx
{
// 생성입니다.
public:
	Ckpaxkkk01Dlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.
	HANDLE m_hSemaphore; // 👈 반드시 클래스 내부에 선언하세요.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_KPAXKKK01_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.
	afx_msg void OnNMCustomdrawListDownload(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnNMRClickListDownload(NMHDR* pNMHDR, LRESULT* pResult); // 우클릭 함수
	afx_msg void OnMenuDelete(); // 삭제 기능 함수


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()


	afx_msg LRESULT OnUpdateProgress(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnDownloadComplete(WPARAM wp, LPARAM lp);
public:
	afx_msg void OnBnClickedBtnStart();
	CListCtrl m_ListCtrl;
	afx_msg void OnBnClickedBtnUpload();
	afx_msg void OnLvnItemchangedListDownload(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedBtnClearFinished();
};
