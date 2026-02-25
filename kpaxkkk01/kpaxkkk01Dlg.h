
// kpaxkkk01Dlg.h: 헤더 파일
//

// kpaxkkk01Dlg.h 상단에 추가
#include <string>
#define WM_UPDATE_PROGRESS (WM_USER + 100)
#define WM_DOWNLOAD_COMPLETE (WM_USER + 101)
#define WM_UPDATE_SPEED (WM_USER + 102) // 속도 업데이트 메시지

// [추가] 스레드 함수 선언 (컴파일러에게 함수의 존재를 알림)
UINT UploadThreadProc(LPVOID pParam);
UINT DownloadThreadProc(LPVOID pParam);





struct DownloadRequest {
	HWND hMainWnd;
	std::wstring source;
	std::wstring target;
	int nItemIndex;
	int nLastPercent;
	DWORD dwLastTick;    // 타임아웃 체크용 (10초)
	DWORD dwSpeedTick;   // [추가] 속도 계산 주기용 (1초 마다 측정)
	LONGLONG nLastBytes; // 이전 전송량 저장용
	LONGLONG nTotalBytes; // 파일의 전체 크기(바이트) 저장용 추가
};


#pragma once


// Ckpaxkkk01Dlg 대화 상자
class Ckpaxkkk01Dlg : public CDialogEx
{
// 생성입니다.
public:
	Ckpaxkkk01Dlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.
	BOOL m_bAllSelected; // 초기값은 FALSE로 설정할 겁니다.
	HANDLE m_hSemaphore; // 👈 반드시 클래스 내부에 선언하세요.

	CStatusBar m_StatusBar; // 상태바 객체
	int m_nTotalFiles;      // 전체 파일 개수
	int m_nDoneFiles;       // 완료된 파일 개수
	CString m_strDownloadPath; // 실제 다운로드 경로 저장

	// 상태 업데이트를 위한 도우미 함수 선언
	void UpdateTotalStatus();

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
	afx_msg LRESULT OnUpdateSpeed(WPARAM wp, LPARAM lp); // 👈 이 줄이 있는지 확인!

// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnNMDblclkListDownload(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()


	afx_msg LRESULT OnUpdateProgress(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnDownloadComplete(WPARAM wp, LPARAM lp);
	afx_msg void OnBnClickedBtnDownloadSelected();
	afx_msg void OnBnClickedBtnSelectAll();
public:
	afx_msg void OnBnClickedBtnStart();
	afx_msg void OnBnClickedBtnUpload();
	afx_msg void OnLvnItemchangedListDownload(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedBtnClearFinished();
	afx_msg void OnBnClickedChkAutoClear();
	afx_msg void ProcessAutoClear();
	afx_msg void OnBnClickedBtnBrowse();
	afx_msg void OnBnClickedBtnDeleteSelected();
	static UINT FtpDownloadThreadProc(LPVOID pParam);
	static UINT FtpUploadThreadProc(LPVOID pParam); // 업로드용 스레드 함수

	/* 리스트목록 폴더화 만들기 */
	CListCtrl m_ListCtrl;
	CImageList m_ImageList;      // 아이콘 저장용
	CString m_strCurrentPath;    // 현재 서버 경로 저장 (예: "/", "/music/")

	afx_msg void OnNMDblclkListMain(NMHDR* pNMHDR, LRESULT* pResult); // 더블클릭 이벤트 함수
};


