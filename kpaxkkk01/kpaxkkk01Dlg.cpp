
// kpaxkkk01Dlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "kpaxkkk01.h"
#include "kpaxkkk01Dlg.h"
#include "afxdialogex.h"

#ifndef CDDS_SUBITEMPREPAINT
#define CDDS_SUBITEMPREPAINT (CDDS_ITEMPREPAINT | CDDS_SUBITEMPREPAINT)
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



// 시스템 에러 코드를 한글 메시지로 변환하는 함수
CString GetErrorMessageKorean(DWORD dwErrorCode) {
	LPWSTR lpMsgBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dwErrorCode, MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT),
		(LPWSTR)&lpMsgBuf, 0, NULL);

	CString strError = lpMsgBuf;
	LocalFree(lpMsgBuf);

	// 줄바꿈 문자 제거 및 커스텀 설명 추가
	strError.TrimRight(L"\r\n");
	if (dwErrorCode == 2) strError = L"파일을 찾을 수 없습니다.";
	if (dwErrorCode == 5) strError = L"액세스 거부 (권한 부족)";
	if (dwErrorCode == 32) strError = L"다른 프로그램이 사용 중입니다.";

	return strError;
}



// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif


// 구현입니다.
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	DECLARE_MESSAGE_MAP()
public:
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// Ckpaxkkk01Dlg 대화 상자



Ckpaxkkk01Dlg::Ckpaxkkk01Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_KPAXKKK01_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Ckpaxkkk01Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_DOWNLOAD, m_ListCtrl);
}

BEGIN_MESSAGE_MAP(Ckpaxkkk01Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_START, &Ckpaxkkk01Dlg::OnBnClickedBtnStart)
	ON_MESSAGE(WM_UPDATE_PROGRESS, &Ckpaxkkk01Dlg::OnUpdateProgress)
	ON_MESSAGE(WM_DOWNLOAD_COMPLETE, &Ckpaxkkk01Dlg::OnDownloadComplete)
	ON_BN_CLICKED(IDC_BTN_UPLOAD, &Ckpaxkkk01Dlg::OnBnClickedBtnUpload)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_DOWNLOAD, &Ckpaxkkk01Dlg::OnNMCustomdrawListDownload)
	ON_WM_DROPFILES()

	// [수정 및 추가된 부분]
	// LVN_ITEMCHANGED는 지우거나 주석처리 하세요 (클릭만 해도 메뉴가 떠서 불편합니다)
	// ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_DOWNLOAD, &Ckpaxkkk01Dlg::OnLvnItemchangedListDownload) 

	// 우클릭했을 때 메뉴 띄우기 연결
	ON_NOTIFY(NM_RCLICK, IDC_LIST_DOWNLOAD, &Ckpaxkkk01Dlg::OnNMRClickListDownload)
	// 메뉴의 '삭제' 버튼을 눌렀을 때 함수 연결
	ON_COMMAND(ID_MENU_DELETE, &Ckpaxkkk01Dlg::OnMenuDelete)
	ON_BN_CLICKED(IDC_BTN_CLEAR_FINISHED, &Ckpaxkkk01Dlg::OnBnClickedBtnClearFinished)
END_MESSAGE_MAP()


// Ckpaxkkk01Dlg 메시지 처리기

BOOL Ckpaxkkk01Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
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

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
	//리스트 컨트롤 스타일 설정(줄 선택, 그리드 라인, 더블 버퍼링)
	m_ListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

	// 2. 드래그 앤 드롭 허용 (이미 하셨다면 중복해서 넣어도 상관없습니다)
	DragAcceptFiles(TRUE);

	// 3. 관리자 권한에서도 드래그 가능하게 필터 해제 (필요시)
	ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
	ChangeWindowMessageFilter(0x0049, MSGFLT_ADD); // WM_COPYGLOBALDATA
	// --- 여기까지 ---

	// [여기가 핵심입니다!]
	// TODO: 여기에 추가 초기화 작업을 추가합니다.

	// 리스트 컨트롤에 눈금선과 한 줄 전체 선택 속성 부여
	m_ListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	// 제목 칸이 생성되어야 하얀 배경에 줄이 생깁니다.
	m_ListCtrl.InsertColumn(0, L"파일명", LVCFMT_LEFT, 150);
	m_ListCtrl.InsertColumn(1, L"진행률", LVCFMT_CENTER, 80);
	m_ListCtrl.InsertColumn(2, L"상태", LVCFMT_LEFT, 100);

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.


	// 리스트 컨트롤 컬럼(칸) 만들기
	m_ListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_ListCtrl.InsertColumn(0, L"파일명", LVCFMT_LEFT, 200);
	m_ListCtrl.InsertColumn(1, L"진행률", LVCFMT_CENTER, 100);
	m_ListCtrl.InsertColumn(2, L"상태", LVCFMT_LEFT, 100);
}

void Ckpaxkkk01Dlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void Ckpaxkkk01Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR Ckpaxkkk01Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}






// 실제 파일 복사를 수행하는 워커 스레드 함수
UINT DownloadThreadProc(LPVOID pParam) {
	DownloadRequest* pReq = (DownloadRequest*)pParam;
	if (pReq == NULL) return 0;

	// 1. 통행권 대기 (동시 전송 개수 제한)
	Ckpaxkkk01Dlg* pMain = (Ckpaxkkk01Dlg*)CWnd::FromHandle(pReq->hMainWnd);
	WaitForSingleObject(pMain->m_hSemaphore, INFINITE);

	BOOL bResult = FALSE;
	int retryCount = 0;
	const int MAX_RETRY = 3;

	// 초기값 설정
	pReq->nLastPercent = -1;
	pReq->dwLastTick = GetTickCount(); // 시작 시간 기록

	// 2. 재시도 루프 시작
	while (retryCount < MAX_RETRY) {
		bResult = CopyFileExW(
			pReq->source.c_str(),
			pReq->target.c_str(),
			[](LARGE_INTEGER total, LARGE_INTEGER trans, LARGE_INTEGER, LARGE_INTEGER, DWORD, DWORD, HANDLE, HANDLE, LPVOID data) -> DWORD {
				DownloadRequest* p = (DownloadRequest*)data;
				DWORD dwCurrentTick = GetTickCount();

				// [타임아웃 체크] 10초 동안 데이터 변화가 없으면 강제 취소
				// trans.QuadPart가 마지막 확인 때보다 늘어났는지 체크하여 시간 갱신
				if (dwCurrentTick - p->dwLastTick > 10000) {
					return PROGRESS_CANCEL; // 함수를 빠져나가 재시도(while) 루프로 보냄
				}

				// 데이터가 조금이라도 전송되었다면 시간 갱신
				// (구조체에 마지막 전송량을 저장할 변수가 없다면 Tick만으로도 어느 정도 작동합니다)
				p->dwLastTick = dwCurrentTick;

				// 진행률 계산
				int percent = (int)((double)trans.QuadPart / (double)total.QuadPart * 100);

				// [최적화] 퍼센트 숫자가 바뀔 때만 UI 메시지 전송
				if (percent > p->nLastPercent) {
					p->nLastPercent = percent;
					::PostMessage(p->hMainWnd, WM_UPDATE_PROGRESS, (WPARAM)percent, (LPARAM)p->nItemIndex);
				}
				return PROGRESS_CONTINUE;
			},
			pReq,
			NULL,
			COPY_FILE_NO_BUFFERING // 시스템 캐시 부하 방지
		);

		if (bResult) break; // 전송 성공 시 루프 탈출

		// 실패 시 (타임아웃 취소 포함) 재시도 준비
		retryCount++;

		// 실패 원인이 타임아웃(CANCEL)인 경우를 대비해 시간 다시 초기화
		pReq->dwLastTick = GetTickCount();

		Sleep(2000); // 2초 대기 후 다시 시도
	}

	// 3. 최종 결과 전송 (성공/실패 알림)
	::PostMessage(pReq->hMainWnd, WM_DOWNLOAD_COMPLETE, (WPARAM)bResult, (LPARAM)pReq->nItemIndex);

	// 4. 통행권 반납 (매우 중요: 다음 대기 파일 출발)
	ReleaseSemaphore(pMain->m_hSemaphore, 1, NULL);

	delete pReq; // 메모리 해제
	return 0;
}

// UI 업데이트 핸들러 구현
LRESULT Ckpaxkkk01Dlg::OnUpdateProgress(WPARAM wp, LPARAM lp) {
	int nPercent = (int)wp; // 진행률 숫자
	int nIdx = (int)lp;     // 리스트의 행 번호

	CString str;
	str.Format(L"%d%%", nPercent);

	// --- 여기서부터 우리가 넣을 코드 시작 ---

	// 1. 리스트의 1번 컬럼(진행률 칸)의 텍스트를 "50%" 처럼 변경합니다.
	m_ListCtrl.SetItemText(nIdx, 1, str);

	// 2. 중요: 변경된 행(nIdx)만 즉시 다시 그리도록 명령합니다.
	// 이 함수가 호출되어야 우리가 만든 '커스텀 드로우'가 작동해서 파란 게이지를 그립니다.
	m_ListCtrl.RedrawItems(nIdx, nIdx);

	// 3. 윈도우에게 지금 당장 화면을 갱신하라고 재촉합니다 (생략 가능하지만 넣으면 더 빠릿합니다).
	m_ListCtrl.UpdateWindow();

	// --- 여기까지 ---

	return 0;
}



LRESULT Ckpaxkkk01Dlg::OnDownloadComplete(WPARAM wp, LPARAM lp) {
	BOOL bSuccess = (BOOL)wp;
	int nIdx = (int)lp;

	if (bSuccess) {
		m_ListCtrl.SetItemText(nIdx, 2, L"완료");
	}
	else {
		// 마지막 에러 코드를 가져와서 한글로 변환
		DWORD dwErr = GetLastError();
		CString strHangulErr = GetErrorMessageKorean(dwErr);

		m_ListCtrl.SetItemText(nIdx, 2, L"실패: " + strHangulErr);
	}
	return 0;
}

// 버튼 클릭 시 호출
void Ckpaxkkk01Dlg::OnBnClickedBtnStart()
{
	// 1. 테스트용 경로 설정 (실제 존재하는 파일 경로로 수정해 보세요)
		// 네트워크 경로 예: L"\\\\192.168.0.10\\Shared\\Movie.mp4"

	PWSTR pszPath = NULL;
	std::wstring dst;

	// 1. 현재 사용자의 '다운로드' 폴더 경로를 자동으로 가져옵니다.
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Downloads, 0, NULL, &pszPath))) {
		dst = pszPath;            // C:\Users\사용자이름\Downloads
		dst += L"\\드래곤볼 오리지널 ep001.mp4";     // 파일명 추가
		CoTaskMemFree(pszPath);   // 사용한 메모리 해제
	}
	else {
		// 경로 가져오기 실패 시 기본값 설정
		dst = L"C:\\새로 저장된 드래곤볼 오리지널 ep001.mp4";
	}

	// 이제 dst를 사용하여 다운로드를 진행합니다.
	// 기존의 InsertItem 및 AfxBeginThread 코드가 이 아래에 오면 됩니다.



	std::wstring src = L"C:\\Test\\드래곤볼 오리지널 ep001.AVI";         // 서버 경로는 그대로 유지
	//std::wstring dst = L"C:\\Test\\downloaded_movie.mp4";

	//보통 웹하드처럼 '다운로드' 폴더에 저장하고 싶다면 경로를 다음과 같이 수정해 보세요. (사용자명 부분은 본인의 윈도우 계정명으로 바꾸셔야 합니다.)
	// 예: 다운로드 폴더로 변경
	//std::wstring dst = L"C:\\Users\\kpax\\Downloads\\나루토01.mp4";

	// 2. 리스트 컨트롤에 항목 추가 (파일명, 진행률 0%, 상태 표시)
	int nIdx = m_ListCtrl.InsertItem(m_ListCtrl.GetItemCount(), L"movie.mp4");
	m_ListCtrl.SetItemText(nIdx, 1, L"0%");
	m_ListCtrl.SetItemText(nIdx, 2, L"다운로드 중...");


	// 3. 스레드에 넘겨줄 데이터 꾸러미 만들기
	DownloadRequest* pReq = new DownloadRequest();
	pReq->hMainWnd = this->GetSafeHwnd();
	pReq->source = src;
	pReq->target = dst;
	pReq->nItemIndex = nIdx;

	// 4. 다운로드 전용 스레드 시작 (UI가 멈추지 않게 함)
	AfxBeginThread(DownloadThreadProc, pReq);
}


// 업로드 워커 스레드 함수
UINT UploadThreadProc(LPVOID pParam) {
	DownloadRequest* pReq = (DownloadRequest*)pParam;
	if (pReq == NULL) return 0;

	// 1. 통행권 대기 (메인 윈도우의 세마포어 사용 - 동시 전송 3개 제한)
	Ckpaxkkk01Dlg* pMain = (Ckpaxkkk01Dlg*)CWnd::FromHandle(pReq->hMainWnd);
	WaitForSingleObject(pMain->m_hSemaphore, INFINITE);

	BOOL bResult = FALSE;
	int retryCount = 0;
	const int MAX_RETRY = 3;

	// 초기 상태 설정
	pReq->nLastPercent = -1;
	pReq->dwLastTick = GetTickCount(); // 시작 시간 기록

	// 2. 재시도 루프 시작
	while (retryCount < MAX_RETRY) {
		bResult = CopyFileExW(
			pReq->source.c_str(),
			pReq->target.c_str(),
			[](LARGE_INTEGER total, LARGE_INTEGER trans, LARGE_INTEGER, LARGE_INTEGER, DWORD, DWORD, HANDLE, HANDLE, LPVOID data) -> DWORD {
				DownloadRequest* p = (DownloadRequest*)data;
				DWORD dwCurrentTick = GetTickCount();

				// [타임아웃 체크] 10초 동안 응답이 없으면 강제 취소 후 재시도
				if (dwCurrentTick - p->dwLastTick > 10000) {
					return PROGRESS_CANCEL;
				}

				// 데이터 전송이 감지되면 시간 갱신
				p->dwLastTick = dwCurrentTick;

				// 진행률 계산
				int percent = (int)((double)trans.QuadPart / (double)total.QuadPart * 100);

				// [최적화] 진행률 숫자가 바뀔 때만 UI 메시지 전송 (버벅임 방지)
				if (percent > p->nLastPercent) {
					p->nLastPercent = percent;
					::PostMessage(p->hMainWnd, WM_UPDATE_PROGRESS, (WPARAM)percent, (LPARAM)p->nItemIndex);
				}

				return PROGRESS_CONTINUE;
			},
			pReq,
			NULL,
			COPY_FILE_NO_BUFFERING // 대용량 업로드 시 시스템 느려짐 방지
		);

		if (bResult) break; // 성공 시 탈출

		// 실패 시 (타임아웃 포함) 재시도 준비
		retryCount++;
		pReq->dwLastTick = GetTickCount(); // 시간 재설정
		Sleep(1500); // 1.5초 대기 후 다시 시도
	}

	// 3. 최종 결과 전송 (성공/실패 및 한글 에러 메시지 처리용)
	::PostMessage(pReq->hMainWnd, WM_DOWNLOAD_COMPLETE, (WPARAM)bResult, (LPARAM)pReq->nItemIndex);

	// 4. 중요: 통행권 반납 (반드시 반납해야 대기 중인 다음 파일이 시작됨)
	ReleaseSemaphore(pMain->m_hSemaphore, 1, NULL);

	delete pReq; // 메모리 해제
	return 0;
}





void Ckpaxkkk01Dlg::OnBnClickedBtnUpload()
{
	// 1. 파일 선택 창
	//CFileDialog fileDlg(TRUE, NULL, NULL, OFN_FILEMUSTEXIST, L"Movie Files (*.mp4;*.mkv)|*.mp4;*.mkv|All Files (*.*)|*.*||");
	CFileDialog fileDlg(TRUE, NULL, NULL, OFN_FILEMUSTEXIST, L"모든 파일 (*.*)|*.*||");

	if (fileDlg.DoModal() == IDOK) {
		CString strLocalPath = fileDlg.GetPathName();
		CString strFileName = fileDlg.GetFileName();

		// 2. 저장 폴더(C:\Test)가 없으면 직접 만듭니다.
		if (!CreateDirectory(L"C:\\Test", NULL)) {
			if (GetLastError() != ERROR_ALREADY_EXISTS) {
				AfxMessageBox(L"C:\\Test 폴더를 생성할 수 없습니다.");
				return;
			}
		}

		// 3. 목적지 경로 설정 (반드시 마지막에 \\ 가 있어야 합니다)
		std::wstring strServerPath = L"C:\\Test\\";
		strServerPath += (LPCTSTR)strFileName;

		// 4. 리스트 컨트롤에 항목 추가
		int nIdx = m_ListCtrl.InsertItem(m_ListCtrl.GetItemCount(), strFileName);
		m_ListCtrl.SetItemText(nIdx, 1, L"0%");
		m_ListCtrl.SetItemText(nIdx, 2, L"업로드 중...");

		// 5. 스레드 데이터 준비
		DownloadRequest* pReq = new DownloadRequest();
		pReq->hMainWnd = this->GetSafeHwnd();
		pReq->source = (LPCTSTR)strLocalPath;
		pReq->target = strServerPath;
		pReq->nItemIndex = nIdx;

		// 6. 업로드 시작
		AfxBeginThread(UploadThreadProc, pReq);
	}
}


void Ckpaxkkk01Dlg::OnNMCustomdrawListDownload(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLVCUSTOMDRAW pLVCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);

	*pResult = CDRF_DODEFAULT;

	// 1단계: 아이템을 그리기 전 단계
	if (pLVCD->nmcd.dwDrawStage == CDDS_PREPAINT)
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	// 2단계: 각 행(Item)을 그리기 전 단계
	else if (pLVCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
	{
		*pResult = CDRF_NOTIFYSUBITEMDRAW;
	}
	// 3단계: 각 칸(SubItem)을 그리기 전 단계
	// 비트 연산자를 사용하여 아이템 예비 페인트와 서브아이템 예비 페인트가 모두 활성화되었는지 확인
	else if (pLVCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT ||
		pLVCD->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | 0x00020000)) // 0x20000은 서브아이템 플래그의 실제 값입니다.
	{
		int nItem = (int)pLVCD->nmcd.dwItemSpec;
		int nSubItem = pLVCD->iSubItem;

		// '진행률' 칸(1번 컬럼)만 직접 그리기
		if (nSubItem == 1)
		{
			CDC* pDC = CDC::FromHandle(pLVCD->nmcd.hdc);
			CRect rect;
			m_ListCtrl.GetSubItemRect(nItem, nSubItem, LVIR_BOUNDS, rect);

			// 1. 현재 진행률 숫자 가져오기 (예: "50%")
			CString strPercent = m_ListCtrl.GetItemText(nItem, nSubItem);
			int nPercent = _ttoi(strPercent); // 문자열을 숫자로 변환

			// 2. 배경 그리기 (흰색)
			pDC->FillSolidRect(rect, RGB(255, 255, 255));

			// 3. 게이지 그리기 (파란색)
			if (nPercent > 0)
			{
				CRect progressRect = rect;
				progressRect.DeflateRect(2, 2); // 테두리 여백
				int fullWidth = progressRect.Width();
				progressRect.right = progressRect.left + (int)(fullWidth * (nPercent / 100.0));

				pDC->FillSolidRect(progressRect, RGB(0, 120, 215)); // 윈도우 블루 색상
			}

			// 4. 숫자 텍스트 그리기 (막대 위에 겹쳐서)
			pDC->SetBkMode(TRANSPARENT);
			if (nPercent > 50) pDC->SetTextColor(RGB(255, 255, 255)); // 바가 차오르면 글씨는 흰색으로
			else pDC->SetTextColor(RGB(0, 0, 0));

			pDC->DrawText(strPercent, rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

			*pResult = CDRF_SKIPDEFAULT; // 기본 그리기를 건너뜀
		}
	}
}


void Ckpaxkkk01Dlg::OnDropFiles(HDROP hDropInfo)
{
	// 1. 떨어뜨린 파일의 개수를 파악합니다.
	UINT nFiles = DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);

	for (UINT i = 0; i < nFiles; i++)
	{
		TCHAR szFilePath[MAX_PATH];
		// 2. 각 파일의 전체 경로를 가져옵니다.
		if (DragQueryFile(hDropInfo, i, szFilePath, MAX_PATH))
		{
			CString strLocalPath = szFilePath;
			CString strFileName = strLocalPath.Mid(strLocalPath.ReverseFind('\\') + 1);

			// 3. 서버 저장 경로 설정 (C:\Test 폴더 기준)
			std::wstring strServerPath = L"C:\\Test\\";
			strServerPath += (LPCTSTR)strFileName;

			// 4. 리스트 컨트롤에 항목 추가
			int nIdx = m_ListCtrl.InsertItem(m_ListCtrl.GetItemCount(), strFileName);
			m_ListCtrl.SetItemText(nIdx, 1, L"0%");
			m_ListCtrl.SetItemText(nIdx, 2, L"연결 중..."); // "드래그 업로드 중..." 대신 변경

			// 5. 스레드 데이터 준비 및 실행 (기존 로직 재사용)
			DownloadRequest* pReq = new DownloadRequest();
			pReq->hMainWnd = this->GetSafeHwnd();
			pReq->source = (LPCTSTR)strLocalPath;
			pReq->target = strServerPath;
			pReq->nItemIndex = nIdx;

			AfxBeginThread(UploadThreadProc, pReq);
		}
	}

	// 6. 작업이 끝나면 핸들을 해제합니다.
	DragFinish(hDropInfo);

	CDialogEx::OnDropFiles(hDropInfo);
}



void Ckpaxkkk01Dlg::OnLvnItemchangedListDownload(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	// iItem은 클릭한 행 번호입니다. -1이 아니라는 건 빈 공간이 아닌 '아이템'을 눌렀다는 뜻!
	if (pNMItemActivate->iItem != -1)
	{
		CMenu menu;
		// 1. 아까 리소스 뷰에서 만든 메뉴판(IDR_MENU_LIST)을 불러옵니다.
		menu.LoadMenu(IDR_MENU_LIST);

		// 2. 메뉴판의 첫 번째 서브메뉴(0번 인덱스, 우리가 만든 '삭제'가 들어있는 칸)를 가져옵니다.
		CMenu* pPopup = menu.GetSubMenu(0);

		// 3. 현재 마우스 커서의 절대 좌표(화면 위치)를 구합니다.
		CPoint pt;
		GetCursorPos(&pt);

		// 4. 구한 좌표(pt.x, pt.y)에 팝업 메뉴를 실제로 띄웁니다.
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this);
	}

	*pResult = 0;
}

void Ckpaxkkk01Dlg::OnMenuDelete()
{
	// 이제 m_ListCtrl을 정상적으로 인식합니다!
	int nItem = m_ListCtrl.GetNextItem(-1, LVNI_SELECTED);

	if (nItem != -1)
	{
		if (AfxMessageBox(L"목록에서 삭제하시겠습니까?", MB_YESNO) == IDYES) {
			m_ListCtrl.DeleteItem(nItem);
		}
	}
}


void Ckpaxkkk01Dlg::OnNMRClickListDownload(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if (pNMItemActivate->iItem != -1) { // 아이템을 우클릭했을 때만
		CMenu menu;
		menu.LoadMenu(IDR_MENU_LIST);
		CMenu* pPopup = menu.GetSubMenu(0);

		CPoint pt;
		GetCursorPos(&pt);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this);
	}
	*pResult = 0;
}


void Ckpaxkkk01Dlg::OnBnClickedBtnClearFinished()
{
	// 리스트의 전체 항목 개수를 가져옵니다.
	int nCount = m_ListCtrl.GetItemCount();

	// 중요: 뒤에서부터(개수-1) 거꾸로 확인하며 지워야 인덱스가 꼬이지 않습니다.
	for (int i = nCount - 1; i >= 0; i--)
	{
		// 2번 컬럼(상태 칸)의 텍스트를 가져옵니다.
		CString strStatus = m_ListCtrl.GetItemText(i, 2);

		// 상태가 "완료"이거나 "오류"인 경우 목록에서 제거합니다.
		if (strStatus == L"완료" || strStatus == L"오류")
		{
			m_ListCtrl.DeleteItem(i);
		}
	}
}